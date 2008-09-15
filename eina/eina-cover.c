#define GEL_DOMAIN "Eina::Cover"
#include <gel/gel.h>
#include <gel/gel-io.h>
#include "eina-cover.h"

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER, EinaCoverPrivate))

#define COVER_H(w) GTK_WIDGET(w)->allocation.height
#define COVER_W(w) COVER_H(w)

typedef struct _EinaCoverPrivate EinaCoverPrivate;

typedef struct {
	const gchar *name;
	EinaCover *self;
	EinaCoverProviderFunc       callback;
	EinaCoverProviderCancelFunc cancel;
	gpointer data;
} EinaCoverBackend;

typedef enum {
	EINA_COVER_BACKEND_STATE_NONE,
	EINA_COVER_BACKEND_STATE_READY,
	EINA_COVER_BACKEND_STATE_RUNNING,
	EINA_COVER_BACKEND_STATE_FAIL,
	EINA_COVER_BACKEND_STATE_SUCCESS,
} EinaCoverBackendState;

typedef struct {
	EinaCoverBackendState state;
	GType                  type;
	gpointer               data;
} EinaCoverBackendData;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;
	gchar      *default_cover;
	LomoStream *stream;

	GHashTable *backends;

	GList *backends_queue;
	GList *current_backend;

	EinaCoverBackendData backend_data;
};

enum {
	EINA_COVER_LOMO_PLAYER_PROPERTY = 1
};

/*
void
eina_cover_set_cover(EinaCover *self, GType type, gpointer data);
static gboolean
eina_cover_provider_check_data(EinaCover *self);
static void
eina_cover_provider_set_data(EinaCover *self, EinaCoverProviderState state, GType type, gpointer data);

*/


static void eina_cover_reset_backends(EinaCover *self);
static void eina_cover_run_backend(EinaCover *self);
void eina_cover_set_backend_data(EinaCover *self, EinaCoverBackendState state, GType type, gpointer data);
EinaCoverBackendData* eina_cover_get_backend_data(EinaCover *self);

void eina_cover_builtin_backend(EinaCover *self, const LomoStream *stream, gpointer data);
void eina_cover_infs_backend(EinaCover *self, const LomoStream *stream, gpointer data);

static void on_eina_cover_lomo_change(LomoPlayer *lomo, gint form, gint to, EinaCover *self);
static void on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self);

static void
eina_cover_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaCover *self = EINA_COVER(object);

	switch (property_id) {
	case EINA_COVER_LOMO_PLAYER_PROPERTY:
		g_value_set_object(value, (gpointer) eina_cover_get_lomo_player(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaCover *self = EINA_COVER(object);

	switch (property_id) {
	case EINA_COVER_LOMO_PLAYER_PROPERTY:
		eina_cover_set_lomo_player(self, LOMO_PLAYER(g_value_get_object(value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_dispose (GObject *object)
{
	EinaCover *self;
	struct _EinaCoverPrivate *priv;

	self = EINA_COVER(object);
	priv = GET_PRIVATE(self);
	
	if (priv != NULL)
	{
		// XXX: Stop backends
		gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);
		gel_free_and_invalidate(priv->default_cover, NULL, g_free);
		gel_free_and_invalidate(priv->backends, NULL, g_hash_table_destroy);
		gel_free_and_invalidate(priv->backends_queue, NULL, g_list_free);
	}

	if (G_OBJECT_CLASS (eina_cover_parent_class)->dispose)
		G_OBJECT_CLASS (eina_cover_parent_class)->dispose (object);
}

static void
eina_cover_class_init (EinaCoverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCoverPrivate));

	object_class->get_property = eina_cover_get_property;
	object_class->set_property = eina_cover_set_property;
	object_class->dispose = eina_cover_dispose;

	g_object_class_install_property(object_class, EINA_COVER_LOMO_PLAYER_PROPERTY,
		g_param_spec_object("lomo-player", "Lomo Player", "Lomo Player",
		LOMO_TYPE_PLAYER,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_cover_init (EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->lomo = NULL;

	priv->default_cover = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "cover-unknow.png");
	if (priv->default_cover == NULL)
	{
		gel_error("Cannot find default cover.");
	}

	priv->backends = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	priv->backends_queue = NULL;
	priv->current_backend = NULL;

	eina_cover_add_provider(self, "default-fallback", eina_cover_builtin_backend, NULL, self);
	eina_cover_add_provider(self, "in-folder",        eina_cover_infs_backend, NULL, self);

	eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_NONE, G_TYPE_INVALID, NULL);
}

EinaCover*
eina_cover_new (void)
{
	return g_object_new (EINA_TYPE_COVER, NULL);
}

void
eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	if ((lomo == NULL) || !LOMO_IS_PLAYER(lomo))
		return;

	g_object_ref(lomo);
	if (priv->lomo != NULL)
		g_object_unref(priv->lomo);
	priv->lomo = lomo;

	g_signal_connect(priv->lomo, "change",
	G_CALLBACK(on_eina_cover_lomo_change), self);
	g_signal_connect(priv->lomo, "clear",
	G_CALLBACK(on_eina_cover_lomo_clear), self);
}

LomoPlayer *
eina_cover_get_lomo_player(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

void
eina_cover_set_cover(EinaCover *self, GType type, gpointer data)
{
	GdkPixbuf *pb;

	if (type == G_TYPE_STRING)
	{
		pb = gdk_pixbuf_new_from_file_at_scale((gchar *) data,
			COVER_W(self), COVER_H(self), FALSE,
			NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(self), pb);
	}

	else if (type == GDK_TYPE_PIXBUF)
	{
		gtk_image_set_from_pixbuf(GTK_IMAGE(self), GDK_PIXBUF(data));
	}

	else
	{
		gel_warn("Invalid type '%s'", g_type_name(type));
	}
}

/*
 * Providers API
 */

// Push provider at provideres queu
void
eina_cover_add_provider(EinaCover *self, const gchar *name,
	EinaCoverProviderFunc callback, EinaCoverProviderCancelFunc cancel,
	gpointer data)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverBackend *backend = g_new0(EinaCoverBackend, 1);
	backend->self     = self;
	backend->name     = name;
	backend->callback = callback;
	backend->cancel   = cancel;
	backend->data     = data;

	g_hash_table_insert(priv->backends, (gpointer) name, backend);
	priv->backends_queue= g_list_prepend(priv->backends_queue, backend);
}

// Remove a provider
void
eina_cover_remove_provider(EinaCover *self, const gchar *name)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	EinaCoverBackend *backend = (EinaCoverBackend*) g_hash_table_lookup(priv->backends, (gpointer) name);
	if (backend == NULL)
	{
		gel_warn("Backend '%s' not found", name);
		return;
	}

	priv->backends_queue = g_list_remove(priv->backends_queue, backend);
	g_hash_table_remove(priv->backends, name);
}

static gboolean
eina_cover_check_backend_state(EinaCover *self)
{
	struct _EinaCoverPrivate *priv =  GET_PRIVATE(self);
	EinaCoverBackendData *bd = eina_cover_get_backend_data(self);
	gel_warn("Current state: %d", bd->state);

	switch(bd->state)
	{
		case EINA_COVER_BACKEND_STATE_READY:
			gel_warn("State is ready, run");
			eina_cover_run_backend(self);
			break;

		case EINA_COVER_BACKEND_STATE_SUCCESS:
			eina_cover_set_cover(self, bd->type, bd->data);
			eina_cover_reset_backends(self);
			break;

		case EINA_COVER_BACKEND_STATE_FAIL:
			gel_warn("Got fail, goto next backend");
			priv->current_backend = priv->current_backend->next;
			eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_READY, G_TYPE_INVALID, NULL);
			eina_cover_run_backend(self);
			break;

		case EINA_COVER_BACKEND_STATE_NONE:
		case EINA_COVER_BACKEND_STATE_RUNNING:
		default:
			gel_warn("Nothing to do in this state %d", bd->state);
	}

	return FALSE;
}

EinaCoverBackendData *
eina_cover_get_backend_data(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return &(priv->backend_data);
}

// Sets data
void eina_cover_set_backend_data(EinaCover *self, EinaCoverBackendState state, GType type, gpointer data)
{
	EinaCoverBackendData *bd =  eina_cover_get_backend_data(self);

	bd->state = state;
	gel_warn("Set state to %d", state);

	// Clear type
	if (bd->type == G_TYPE_INVALID)
	{
		gel_warn("Stored data is already invalid");
	}
	else if (bd->type == G_TYPE_STRING)
	{
		gel_warn("Clear backend data %p of type G_TYPE_STRING (%s)", bd->data, bd->data);
		g_free(bd->data);
		bd->data = NULL;
		bd->type = G_TYPE_INVALID;
	}
	else
	{
		gel_warn("Unknow type to clear");
	}

	// Set new data
	if (type == G_TYPE_INVALID)
	{
		gel_warn("Invalidating internal data");
		bd->type = G_TYPE_INVALID;
		bd->data = NULL;
	}
	else if (type == G_TYPE_STRING)
	{
		bd->data = g_strdup(data);
		bd->type = type;
		gel_warn("Set backend data to %p (%s), type G_TYPE_STRING", bd->data, bd->data);
	}
	else
	{
		gel_warn("Invalid data to store, unknow type %s", g_type_name(type));
	}
}


static void
eina_cover_reset_backends(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverBackend *backend;

	if (priv->current_backend != NULL)
	{
		gel_warn("Active backend, call cancel backend");
		backend = (EinaCoverBackend *) priv->current_backend->data;
		if (backend->cancel)
			backend->cancel(self, backend->data);
	}

	priv->current_backend = priv->backends_queue;
	eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_READY, G_TYPE_INVALID, NULL);
}

static void
eina_cover_run_backend(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverBackend *backend;
	EinaCoverBackendData bd = priv->backend_data;

	if (bd.state != EINA_COVER_BACKEND_STATE_READY)
	{
		gel_warn("Cant run backend, not in ready mode");
		return;
	}

	if ((priv->current_backend == NULL) || (priv->current_backend->data == NULL))
	{
		gel_warn("No more backends, go to none state");
		eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_NONE, G_TYPE_INVALID, NULL);
		return;
	}

	eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_RUNNING, G_TYPE_INVALID, NULL);
	backend = (EinaCoverBackend *) priv->current_backend->data;
	backend->callback(self, priv->stream, backend->data);
}

void
eina_cover_backend_success(EinaCover *self, GType type, gpointer data)
{
	eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_SUCCESS, type, data);
	g_idle_add((GSourceFunc) eina_cover_check_backend_state, self);
}

void
eina_cover_backend_fail(EinaCover *self)
{
	eina_cover_set_backend_data(self, EINA_COVER_BACKEND_STATE_FAIL, G_TYPE_INVALID, NULL);
	g_idle_add((GSourceFunc) eina_cover_check_backend_state, self);
}

static void
on_eina_cover_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	const LomoStream *stream;
	if (to == -1)
	{
		gel_debug("Change to -1, reset cover");
		eina_cover_backend_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
		return;
	}

	if ((stream = lomo_player_get_nth(lomo, to)) == NULL)
	{
		gel_warn("Stream no. %d is NULL. reset cover");
		eina_cover_backend_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
		return;
	}

	g_object_ref(G_OBJECT(stream));
	priv->stream = LOMO_STREAM(stream);

	gel_warn("Stream changed to %p (%d -> %d)", stream, from, to);

/*
	if (from != -1)
		eina_cover_providers_stop(self);
	eina_cover_providers_start(self);
*/
	eina_cover_reset_backends(self);
	eina_cover_run_backend(self);
}

static void
on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->stream, NULL, g_object_unref);
	eina_cover_backend_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
}

/* Build-in provider */
void eina_cover_builtin_backend(EinaCover *self, const LomoStream *stream, gpointer data)
{
	EinaCover *obj = EINA_COVER(data);
	struct _EinaCoverPrivate *priv = GET_PRIVATE(obj);

	eina_cover_backend_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
}

void eina_cover_infs_backend(EinaCover *self, const LomoStream *stream, gpointer data)
{
	gchar *uri = lomo_stream_get_tag(LOMO_STREAM(stream), LOMO_TAG_URI);
	gchar *pathname = g_filename_from_uri(uri, NULL, NULL);
	gchar *dirname, *coverfile;

	if (g_random_int()%2)
	{
		g_free(pathname);
		eina_cover_backend_fail(self);
	}
	else
	{
		dirname = g_path_get_dirname(pathname);
		g_free(pathname);

		coverfile = g_build_filename(dirname, "cover.jpg", NULL);
		g_free(dirname);

		if (g_file_test(coverfile, G_FILE_TEST_IS_REGULAR))
			eina_cover_backend_success(self, G_TYPE_STRING, coverfile);
		else
		{
			g_free(coverfile);
			eina_cover_backend_fail(self);
		}
	}
}
