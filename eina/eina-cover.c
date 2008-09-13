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
} EinaCoverProvider;

typedef enum {
	EINA_COVER_PROVIDER_STATE_NONE,
	EINA_COVER_PROVIDER_STATE_READY,
	EINA_COVER_PROVIDER_STATE_RUNNING,
	EINA_COVER_PROVIDER_STATE_FAIL,
	EINA_COVER_PROVIDER_STATE_SUCCESS,
} EinaCoverProviderState;

typedef struct {
	EinaCoverProviderState state;
	GType                  type;
	gpointer               data;
} EinaCoverProviderData;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;
	gchar      *default_cover;
	LomoStream *stream;

	GHashTable *providers;

	GList *providers_queue;
	GList *active_provider;

	EinaCoverProviderData provider_data;
};

enum {
	EINA_COVER_LOMO_PLAYER_PROPERTY = 1
};

void
eina_cover_set_cover(EinaCover *self, GType type, gpointer data);
static gboolean
eina_cover_provider_check_data(EinaCover *self);
static void
eina_cover_provider_set_data(EinaCover *self, EinaCoverProviderState state, GType type, gpointer data);

static void on_eina_cover_lomo_change(LomoPlayer *lomo, gint form, gint to, EinaCover *self);
static void on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self);

void eina_cover_builtin_provider(EinaCover *self, const LomoStream *stream, gpointer data);
void eina_cover_infs_provider(EinaCover *self, const LomoStream *stream, gpointer data);

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
		gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);
		gel_free_and_invalidate(priv->default_cover, NULL, g_free);
		gel_free_and_invalidate(priv->providers, NULL, g_hash_table_destroy);
		gel_free_and_invalidate(priv->providers_queue, NULL, g_list_free);
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

	priv->providers       = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	priv->providers_queue = NULL;
	priv->active_provider = NULL;

	eina_cover_add_provider(self, "default-fallback", eina_cover_builtin_provider, NULL, self);
	eina_cover_add_provider(self, "in-folder",        eina_cover_infs_provider, NULL, self);
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
		g_free(data);
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
	EinaCoverProvider *pack = g_new0(EinaCoverProvider, 1);
	pack->self     = self;
	pack->name     = name;
	pack->callback = callback;
	pack->cancel   = cancel;
	pack->data     = data;

	g_hash_table_insert(priv->providers, (gpointer) name, pack);
	priv->providers_queue = g_list_prepend(priv->providers_queue, pack);
}

// Remove a provider
void
eina_cover_remove_provider(EinaCover *self, const gchar *name)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	EinaCoverProvider * pack = (EinaCoverProvider *) g_hash_table_lookup(priv->providers, (gpointer) name);
	if (pack == NULL)
	{
		gel_warn("Provider '%s' not found", name);
		return;
	}

	priv->providers_queue = g_list_remove(priv->providers_queue, pack);
	g_hash_table_remove(priv->providers, name);
}

// Start processing providers queue
static void
eina_cover_providers_start(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (priv->active_provider != NULL)
	{
		gel_error("There is an active provider");
		return;
	}

	if ((priv->active_provider = priv->providers_queue) == NULL)
	{
		gel_error("No providers available");
		return;
	}

	gel_warn("Starting providers");
	eina_cover_provider_set_data(self, EINA_COVER_PROVIDER_STATE_READY, G_TYPE_INVALID, NULL);
	eina_cover_provider_check_data(self);
	// g_idle_add((GSourceFunc) eina_cover_provider_check_data, self);
}

static void
eina_cover_providers_stop(EinaCover *self)
{
    struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProvider * pack;

	if (priv->active_provider == NULL)
	{
		gel_warn("Cannot stop a NULL provider");
		eina_cover_provider_set_data(self, EINA_COVER_PROVIDER_STATE_NONE, G_TYPE_INVALID, NULL);
		return;
	}

	pack = (EinaCoverProvider *) priv->active_provider->data;
	if (pack->cancel == NULL)
		gel_warn("Provider dont have a cancel callback");
	else
		pack->cancel(self, pack->data);

	priv->active_provider = NULL;
	gel_warn("Stopping providers");
}

static void
eina_cover_providers_run(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProvider *pack;

	if (priv->active_provider == NULL)
	{
		gel_warn("  Cannot run a NULL provider");
		return;
	}

	pack = (EinaCoverProvider *) priv->active_provider->data;
	if ((pack == NULL) || (pack->callback == NULL))
	{
		gel_warn("  Invalid provider %p callback %p to run", pack, (pack == NULL) ? NULL : pack->callback);
		return;
	}

	gel_warn("  Run provider '%s'", pack->name);
	eina_cover_provider_set_data(self, EINA_COVER_PROVIDER_STATE_RUNNING, G_TYPE_INVALID, NULL);

	pack->callback(self, priv->stream, pack->data);
}

static void
eina_cover_providers_go_next(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProvider *pack;

	if (priv->active_provider == NULL)
	{
		gel_warn("  Cannot go next from NULL provider");
		eina_cover_providers_stop(self);
		return;
	}

	if ((priv->active_provider = priv->active_provider->next) == NULL)
	{
		gel_warn("  No next provider, stop");
		eina_cover_providers_stop(self);
		return;
	}

	pack = (EinaCoverProvider *) priv->active_provider->data;
	if ((pack == NULL) || (pack->callback == NULL))
	{
		gel_warn("  Invalid provider %p callback %p to go", pack, (pack == NULL) ? NULL : pack->callback);
		eina_cover_providers_go_next(self);
		return;
	}

	gel_warn("  Moved to next provider");
	eina_cover_providers_run(self);
}

static gboolean
eina_cover_provider_check_data(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProviderData data = priv->provider_data;
	switch (data.state)
	{
	case EINA_COVER_PROVIDER_STATE_READY:
		eina_cover_providers_run(self);
		break;

	case EINA_COVER_PROVIDER_STATE_FAIL:
		eina_cover_providers_go_next(self);
		break;

	case EINA_COVER_PROVIDER_STATE_SUCCESS:
		gel_warn("   Last provider got results: '%s'@%p", g_type_name(data.type), data.data);
		eina_cover_set_cover(self, data.type, data.data);
		eina_cover_providers_stop(self);
		break;

	case EINA_COVER_PROVIDER_STATE_NONE:
	case EINA_COVER_PROVIDER_STATE_RUNNING:
	default:
		gel_error("   We shouldn't be called in this state");
		break;

	}
	return FALSE;
}

static void
eina_cover_provider_set_data(EinaCover *self, EinaCoverProviderState state, GType type, gpointer data)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProviderData pdata = priv->provider_data;

	// Clear previous
	if ((pdata.type == G_TYPE_STRING) && (pdata.data != NULL)) 
	{
		g_free(pdata.data);
		pdata.data = NULL;
	}

	priv->provider_data.state = state;
	priv->provider_data.type  = type;
	priv->provider_data.data  = data;
}

/*
static EinaCoverProviderData
eina_cover_provider_get_data(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return priv->provider_data;
}
*/

void
eina_cover_provider_success(EinaCover *self, GType type, gpointer data)
{
	eina_cover_provider_set_data(self, EINA_COVER_PROVIDER_STATE_SUCCESS, type, data);
	eina_cover_provider_check_data( self);
	// g_idle_add((GSourceFunc) eina_cover_provider_check_data, self);
}

void
eina_cover_provider_fail(EinaCover *self)
{
	eina_cover_provider_set_data(self, EINA_COVER_PROVIDER_STATE_FAIL, G_TYPE_INVALID, NULL);
	eina_cover_provider_check_data( self);
	// g_idle_add((GSourceFunc) eina_cover_provider_check_data, self);
}

static void
on_eina_cover_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	const LomoStream *stream;
	if (to == -1)
	{
		gel_debug("Change to -1, reset cover");
		eina_cover_provider_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
		return;
	}

	if ((stream = lomo_player_get_nth(lomo, to)) == NULL)
	{
		gel_warn("Stream no. %d is NULL. reset cover");
		eina_cover_provider_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
		return;
	}

	g_object_ref(G_OBJECT(stream));
	priv->stream = LOMO_STREAM(stream);

	gel_warn("Stream changed to %p (%d -> %d)", stream, from, to);

	if (from != -1)
		eina_cover_providers_stop(self);
	eina_cover_providers_start(self);
}

static void
on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->stream, NULL, g_object_unref);
	eina_cover_provider_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
}

/* Build-in provider */
void eina_cover_builtin_provider(EinaCover *self, const LomoStream *stream, gpointer data)
{
	EinaCover *obj = EINA_COVER(data);
	struct _EinaCoverPrivate *priv = GET_PRIVATE(obj);

	eina_cover_provider_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
}

void eina_cover_infs_provider(EinaCover *self, const LomoStream *stream, gpointer data)
{
	gchar *uri = lomo_stream_get_tag(LOMO_STREAM(stream), LOMO_TAG_URI);
	gchar *pathname = g_filename_from_uri(uri, NULL, NULL);
	gchar *dirname, *coverfile;

	if (g_random_int()%2)
	{
		g_free(pathname);
		eina_cover_provider_fail(self);
	}
	else
	{
		dirname = g_path_get_dirname(pathname);
		g_free(pathname);

		coverfile = g_build_filename(dirname, "cover.jpg", NULL);
		g_free(dirname);

		if (g_file_test(coverfile, G_FILE_TEST_IS_REGULAR))
			eina_cover_provider_success(self, G_TYPE_STRING, coverfile);
		else
		{
			g_free(coverfile);
			eina_cover_provider_fail(self);
		}
	}
}
