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

struct EinaCoverProviderPack {
	EinaCover *self;
	EinaCoverProviderFunc       callback;
	EinaCoverProviderCancelFunc cancel;
	gpointer data;
};

struct _EinaCoverPrivate {
	LomoPlayer *lomo;
	gchar *default_cover;

	GHashTable *providers;
	GList *providers_order;
	GList *active_provider;
};

enum {
	EINA_COVER_LOMO_PLAYER_PROPERTY = 1
};

static void eina_cover_set_from_filename(EinaCover *self, gchar *filename);
static void eina_cover_set_data(EinaCover *self, gchar *artist, gchar *album);
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
		gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);
		gel_free_and_invalidate(priv->default_cover, NULL, g_free);
		gel_free_and_invalidate(priv->providers, NULL, g_hash_table_destroy);
		gel_free_and_invalidate(priv->providers_order, NULL, g_list_free);
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
	priv->providers_order = NULL;
	priv->active_provider = priv->providers_order;
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
	{
		gel_warn("Invalid lomo player object");
		return;
	}

	g_object_ref(lomo);
	if (priv->lomo != NULL)
		g_object_unref(priv->lomo);
	priv->lomo = lomo;

	g_signal_connect(priv->lomo, "change",
	G_CALLBACK(on_eina_cover_lomo_change), self);
	g_signal_connect(priv->lomo, "clear",
	G_CALLBACK(on_eina_cover_lomo_clear), self);
	gel_info("LomoPlayer is in da houze");
}

LomoPlayer *
eina_cover_get_lomo_player(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

void
eina_cover_add_provider(EinaCover *self, const gchar *name,
	EinaCoverProviderFunc callback, EinaCoverProviderCancelFunc cancel,
	gpointer data)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	struct EinaCoverProviderPack *pack = g_new0(struct EinaCoverProviderPack, 1);
	pack->self = self;
	pack->callback = callback;
	pack->cancel = cancel;
	pack->data = data;

	g_hash_table_insert(priv->providers, (gpointer) name, pack);
	priv->providers_order = g_list_prepend(priv->providers_order, pack);
}

void
eina_cover_remove_provider(EinaCover *self, const gchar *name)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	struct EinaCoverProviderPack * pack = (struct EinaCoverProviderPack *) g_hash_table_lookup(priv->providers, (gpointer) name);
	if (pack == NULL)
	{
		gel_warn("Provider '%s' not found", name);
		return;
	}

	priv->providers_order = g_list_remove(priv->providers_order, pack);
	g_hash_table_remove(priv->providers, name);
}

#if 0
void
eina_cover_add_provider(EinaCover *self, const gchar *name, guint weight, EinaCoverProviderFunc callback, gpointer data)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	GList *nth;

	if (callback == NULL)
		return;

	// Insert into hash
	g_hash_table_insert(priv->providers, (gpointer) name, (gpointer) callback);
	g_hash_table_insert(priv->providers_data, (gpointer) name, data);

	// Add to list
	nth = g_list_nth(priv->providers_order, weight);
	if (nth == NULL)
		nth = priv->providers_order;

	priv->providers_order = g_list_insert_before(priv->providers_order, nth, (gpointer) name);
}

void
eina_cover_delete_provider(EinaCover *self, const gchar *name)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	EinaCoverProviderFunc callback;

	// Search callback address
	callback = (EinaCoverProviderFunc) g_hash_table_lookup(priv->providers, (gpointer) name);
	if (callback == NULL)
		return;

	// Remove callback and data
	priv->providers_order = g_list_remove(priv->providers_order, (gpointer) callback);
	g_hash_table_remove(priv->providers,      (gpointer) name);
	g_hash_table_remove(priv->providers_data, (gpointer) name);
}
#endif

void
eina_cover_provider_try_next(EinaCover *self)
{
}

void
eina_cover_provider_success(EinaCover *self, gchar *uri)
{
}

void eina_cover_reset(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);

	gel_info("Resetting.");
	eina_cover_set_from_filename(self, priv->default_cover);
}

void eina_cover_set_data(EinaCover *self, gchar *artist, gchar *album)
{
	if ((artist == NULL) && (album == NULL))
	{
		gel_warn("Neither artist or album data found, reset.");
		eina_cover_reset(self);
		return;
	}

	gel_info("Setting data to: '%s' '%s'", artist, album);
	eina_cover_reset(self);
}

static void
eina_cover_set_from_filename(EinaCover *self, gchar *filename)
{
	GdkPixbuf *pb;
	GError *error = NULL;

	pb = gdk_pixbuf_new_from_file_at_scale(filename, COVER_W(self), COVER_H(self),
		TRUE, &error);

	if (pb == NULL)
	{
		gel_warn("Cannot load '%s': '%s'", filename, error->message);
		g_error_free(error);
		return;
	}
	gel_info("Loading cover from '%s' at %dx%d", filename, COVER_W(self), COVER_H(self));
	gtk_image_set_from_pixbuf(GTK_IMAGE(self), pb);
}

static void
on_eina_cover_lomo_change(LomoPlayer *lomo, gint form, gint to, EinaCover *self)
{
	const LomoStream *stream;
	gchar *artist = NULL, *album = NULL;
	if (to == -1)
	{
		gel_debug("Change to -1, reset cover");
		return;
	}

	if ((stream = lomo_player_get_nth(lomo, to)) == NULL)
	{
		gel_warn("Stream no. %d is NULL. reset cover");
		return;
	}

	artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	gel_debug("Stream changed to %p, Artist: '%s', album '%s'", stream, artist, album);
	eina_cover_set_data(self, artist, album);
}

static void
on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self)
{
	gel_debug("Clearing, reset cover");
}

