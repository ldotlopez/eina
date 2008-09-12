#define GEL_DOMAIN "Eina::Cover"
#include <gel/gel.h>
#include "eina-cover.h"

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER, EinaCoverPrivate))

#define COVER_H(w) GTK_WIDGET(w)->allocation.height
#define COVER_W(w) COVER_H(w)

typedef struct _EinaCoverPrivate EinaCoverPrivate;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;
	gchar *default_cover;
};

enum {
	EINA_COVER_LOMO_PLAYER_PROPERTY = 1
};

static void eina_cover_set_from_filename(EinaCover *self, gchar *filename);
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
}

static void
eina_cover_init (EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->lomo = NULL;
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

void eina_cover_reset(EinaCover *self)
{
	struct _EinaCoverPrivate *priv = GET_PRIVATE(self);
	eina_cover_set_from_filename(self, priv->default_cover);
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

	gtk_image_set_from_pixbuf(GTK_IMAGE(self), pb);
}

static void on_eina_cover_lomo_change(LomoPlayer *lomo, gint form, gint to, EinaCover *self)
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
}

static void on_eina_cover_lomo_clear(LomoPlayer *lomo, EinaCover *self)
{
	gel_debug("Clearing, reset cover");
}

