#define GEL_DOMAIN "Eina::Cover"
#include <gel/gel.h>
#include <gel/gel-io.h>
#include "eina-cover.h"

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER, EinaCoverPrivate))

typedef struct _EinaCoverPrivate EinaCoverPrivate;

typedef struct {
	const gchar *name;
	EinaCover *self;
	EinaCoverBackendFunc       callback;
	EinaCoverBackendCancelFunc cancel;
	gpointer data;
} EinaCoverBackend;

typedef enum {
	EINA_COVER_BACKEND_STATE_NONE,
	EINA_COVER_BACKEND_STATE_READY,
	EINA_COVER_BACKEND_STATE_RUNNING,
} EinaCoverBackendState;

typedef struct {
	EinaCoverBackendState state;
	GType                  type;
	gpointer               data;
} EinaCoverBackendData;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;
	gchar      *default_cover;
	gchar      *loading_cover;
	LomoStream *stream;

	gboolean found;

	GHashTable *backends;

	GList *backends_queue;
	GList *current_backend;
	EinaCoverBackendState backend_state;
};

enum {
	EINA_COVER_LOMO_PLAYER_PROPERTY = 1,
	EINA_COVER_DEFAULT_COVER_PROPERTY,
	EINA_COVER_LOADING_COVER_PROPERTY
};

enum {
	CHANGE,
	LAST_SIGNAL
};
static guint eina_cover_signals[LAST_SIGNAL] = { 0 };

gboolean eina_cover_set_cover(EinaCover *self, GType type, gpointer data);
void     eina_cover_run_backends(EinaCover *self);
void     eina_cover_reset_backends(EinaCover *self);

static void lomo_change_cb(LomoPlayer *lomo, gint form, gint to, EinaCover *self);
static void lomo_clear_cb(LomoPlayer *lomo, EinaCover *self);
static void lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaCover *self);

static void
eina_cover_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaCover *self = EINA_COVER(object);

	switch (property_id) {
	case EINA_COVER_LOMO_PLAYER_PROPERTY:
		g_value_set_object(value, (gpointer) eina_cover_get_lomo_player(self));
		break;

	case EINA_COVER_DEFAULT_COVER_PROPERTY:
		g_value_set_string(value, eina_cover_get_default_cover(self));
		break;

	case EINA_COVER_LOADING_COVER_PROPERTY:
		g_value_set_string(value, (gpointer) eina_cover_get_loading_cover(self));
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

	case EINA_COVER_DEFAULT_COVER_PROPERTY:
		eina_cover_set_default_cover(self, (gchar *) g_value_get_string(value));
		break;

	case EINA_COVER_LOADING_COVER_PROPERTY:
		eina_cover_set_loading_cover(self, (gchar *) g_value_get_string(value));
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
	g_object_class_install_property(object_class, EINA_COVER_DEFAULT_COVER_PROPERTY,
		g_param_spec_string("default-cover", "Default cover", "Default cover",
		NULL,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, EINA_COVER_LOADING_COVER_PROPERTY,
		g_param_spec_string("loading-cover", "Loading cover", "Loading cover",
		NULL,  G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	eina_cover_signals[CHANGE] = g_signal_new ("change",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaCoverClass, change),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,
		0);
}

static void
eina_cover_init (EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->lomo = NULL;

	priv->default_cover = priv->loading_cover = NULL;
	priv->found = FALSE;

	priv->backends = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	priv->backends_queue = NULL;
	priv->current_backend = NULL;
	priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
}

EinaCover*
eina_cover_new (void)
{
	return g_object_new (EINA_TYPE_COVER, NULL);
}

EinaCover*
eina_cover_new_with_opts(gchar *default_cover, gchar *loading_cover)
{
	EinaCover *self = eina_cover_new();
	eina_cover_set_default_cover(self, default_cover);
	eina_cover_set_loading_cover(self, loading_cover);
	return self;
}

void
eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	if ((lomo == NULL) || !LOMO_IS_PLAYER(lomo))
		return;

	g_object_ref(lomo);
	if (priv->lomo != NULL)
	{
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_change_cb, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_all_tags_cb, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_clear_cb, self);
		g_object_unref(priv->lomo);
	}

	priv->lomo = lomo;
	g_signal_connect(priv->lomo, "change",
	G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(priv->lomo, "all-tags",
	G_CALLBACK(lomo_all_tags_cb), self);
	g_signal_connect(priv->lomo, "clear",
	G_CALLBACK(lomo_clear_cb), self);
}

LomoPlayer *
eina_cover_get_lomo_player(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

void
eina_cover_set_default_cover(EinaCover *self, gchar *filename)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (filename == NULL)
		return;

	if (priv->default_cover != NULL)
		g_free(priv->default_cover);
	priv->default_cover = g_strdup(filename);
	if (priv->backend_state != EINA_COVER_BACKEND_STATE_RUNNING)
		eina_cover_set_cover(self, G_TYPE_STRING, priv->default_cover);
}

gchar *
eina_cover_get_default_cover(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return g_strdup(priv->default_cover);
}

void
eina_cover_set_loading_cover(EinaCover *self, gchar *filename)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (filename == NULL)
		return;

	if (priv->loading_cover != NULL)
		g_free(priv->loading_cover);
	priv->loading_cover = g_strdup(filename);
}

gchar *
eina_cover_get_loading_cover(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return g_strdup(priv->loading_cover);
}

gboolean
eina_cover_set_cover(EinaCover *self, GType type, gpointer data)
{
	GdkPixbuf *pb, *orig;
	GtkRequisition req;
	// double sx, sy;

	if (type == G_TYPE_STRING)
	{
		gtk_widget_size_request(GTK_WIDGET(self), &req);
		pb = gdk_pixbuf_new_from_file_at_scale((gchar *) data,
			req.width, req.height, FALSE,
			NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(self), pb);
		g_signal_emit(self, eina_cover_signals[CHANGE], 0);

		return TRUE;
	}

	else if (type == GDK_TYPE_PIXBUF)
	{
		orig = GDK_PIXBUF(data);
		gtk_widget_size_request(GTK_WIDGET(self), &req);
		/*
		sx = COVER_W(self) / gdk_pixbuf_get_width(orig);
		sy = COVER_H(self) / gdk_pixbuf_get_height(orig);
		(sx > sy) ? (sx = sy) : (sy = sx) ;

		pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, gdk_pixbuf_get_bits_per_sample(orig),
			COVER_W(self), COVER_H(self));
		gdk_pixbuf_scale(orig, pb,
			0, 0,
			COVER_W(self), COVER_H(self),
			0, 0,
			sx, sy,
			GDK_INTERP_TILES);
		*/
		pb = gdk_pixbuf_scale_simple(orig, req.width, req.height, GDK_INTERP_TILES);
		gtk_image_set_from_pixbuf(GTK_IMAGE(self), GDK_PIXBUF(pb));
		g_signal_emit(self, eina_cover_signals[CHANGE], 0);
		return TRUE;
	}

	else
	{
		gel_warn("Invalid type '%s'", g_type_name(type));
		return FALSE;
	}
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	const LomoStream *stream;
	if (to == -1)
	{
		eina_cover_backend_success(self, G_TYPE_STRING, g_strdup(priv->default_cover));
		return;
	}

	if ((stream = lomo_player_get_nth(lomo, to)) == NULL)
	{
		eina_cover_backend_success(self, G_TYPE_STRING, priv->default_cover);
		return;
	}

	g_object_ref(G_OBJECT(stream));
	priv->stream = LOMO_STREAM(stream);

	if (from == to)
		return;

	eina_cover_set_cover(self, G_TYPE_STRING, priv->loading_cover);
	eina_cover_reset_backends(self);
	eina_cover_run_backends(self);
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->stream, NULL, g_object_unref);
	eina_cover_backend_success(self, G_TYPE_STRING, priv->default_cover);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->stream != stream)
		return;

	return;
	/*
	gel_warn("Got all tags on '%s'", (gchar *) lomo_stream_get_tag(stream, LOMO_TAG_URI));
	if (priv->found == TRUE)
		gel_warn("Got all-tags signal but cover is already found");
	else
		gel_warn("Got all-tags signal, re-try cover search");
	*/
}

/*
 * Backends API
 */

// Push provider at provideres queue
void
eina_cover_add_backend(EinaCover *self, const gchar *name,
	EinaCoverBackendFunc callback, EinaCoverBackendCancelFunc cancel,
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
	priv->backends_queue = g_list_append(priv->backends_queue, backend);

	if (priv->backend_state != EINA_COVER_BACKEND_STATE_NONE)
	{
		priv->current_backend = g_list_last(priv->backends_queue);
		priv->backend_state = EINA_COVER_BACKEND_STATE_READY;
		eina_cover_run_backends(self);
	}
}

// Remove a provider
void
eina_cover_remove_backend(EinaCover *self, const gchar *name)
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

void
eina_cover_stop_backend(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverBackend *backend;

	if (priv->backend_state != EINA_COVER_BACKEND_STATE_RUNNING)
	{
		gel_warn("BUG: No backend running");
		return;
	}

	backend = (EinaCoverBackend *) priv->current_backend->data;
	if (backend == NULL)
	{
		gel_warn("BUG: Invalid backend");
	}

	// gel_warn("Canceling backend '%s'", backend->name);
	if (backend->cancel != NULL)
		backend->cancel(self, backend->data);
}

void
eina_cover_reset_backends(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->backend_state == EINA_COVER_BACKEND_STATE_RUNNING)
		eina_cover_stop_backend(self);

	priv->current_backend = g_list_last(priv->backends_queue);
	priv->backend_state = EINA_COVER_BACKEND_STATE_READY;
}

void
eina_cover_run_backends(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverBackend *backend;

	if ((priv->current_backend == NULL))
	{
		gel_warn("BUG: no current backend but available backends %d", g_list_length(priv->backends_queue));
		eina_cover_set_cover(self, G_TYPE_STRING, priv->default_cover);
		return;
	}

	if (priv->backend_state != EINA_COVER_BACKEND_STATE_READY)
	{
		gel_warn("BUG: not in ready state");
		eina_cover_set_cover(self, G_TYPE_STRING, priv->default_cover);
		return;
	}

	backend = (EinaCoverBackend *) priv->current_backend->data;
	if ((backend == NULL) || (backend->callback == NULL))
	{
		gel_warn("BUG: invalid backend");
		priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
		priv->current_backend = NULL;
		eina_cover_set_cover(self, G_TYPE_STRING, priv->default_cover);
		return;
	}

	 priv->backend_state = EINA_COVER_BACKEND_STATE_RUNNING;
	 backend->callback(self, priv->stream, backend->data);
}

gboolean
eina_cover_backend_fail_real(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (priv->current_backend == NULL)
	{
		gel_warn("BUG: Current backend is NULL");
		priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
		return FALSE;
	}

	if (priv->backend_state == EINA_COVER_BACKEND_STATE_RUNNING)
	{
		gel_warn("BUG: Backend is still running");
		return FALSE;
	}

	priv->current_backend = priv->current_backend->prev;
	if (priv->current_backend == NULL)
	{
		priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
		eina_cover_set_cover(self, G_TYPE_STRING, priv->default_cover);
	}
	else
	{
		priv->backend_state = EINA_COVER_BACKEND_STATE_READY;
		eina_cover_run_backends(self);
	}
	return FALSE;
}

void
eina_cover_backend_fail(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
	g_idle_add((GSourceFunc) eina_cover_backend_fail_real, (gpointer) self);
}

void
eina_cover_backend_success(EinaCover *self, GType type, gpointer data)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (!eina_cover_set_cover(self, type, data))
	{
		eina_cover_backend_fail(self);
		return;
	}
	else
	{
		priv->backend_state = EINA_COVER_BACKEND_STATE_NONE;
		priv->current_backend = NULL;
	}
}

