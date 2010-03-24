#include "eina-cover-clutter.h"

G_DEFINE_TYPE (EinaCoverClutter, eina_cover_clutter, GTK_CLUTTER_TYPE_EMBED)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_CLUTTER, EinaCoverClutterPrivate))

typedef struct _EinaCoverClutterPrivate EinaCoverClutterPrivate;

struct _EinaCoverClutterPrivate {
		int dummy;
};

enum {
	PROPERTY_COVER = 1
};

static void
eina_cover_clutter_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_COVER:
		eina_cover_clutter_set_from_pixbuf(EINA_COVER_CLUTTER(object), GDK_PIXBUF(g_value_get_object(value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_cover_clutter_parent_class)->dispose (object);
}

static void
eina_cover_clutter_class_init (EinaCoverClutterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCoverClutterPrivate));

	object_class->get_property = eina_cover_clutter_get_property;
	object_class->set_property = eina_cover_clutter_set_property;
	object_class->dispose = eina_cover_clutter_dispose;

    g_object_class_install_property(object_class, PROPERTY_COVER,
		g_param_spec_object("cover", "Cover", "Cover pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	gint r = gtk_clutter_init(NULL, NULL);
	if (r != CLUTTER_INIT_SUCCESS)
		g_warning("Unable to init clutter-gtk: %d", r);
}

static void
eina_cover_clutter_init (EinaCoverClutter *self)
{
}

EinaCoverClutter*
eina_cover_clutter_new (void)
{
	return g_object_new (EINA_TYPE_COVER_CLUTTER, NULL);
}

void
eina_cover_clutter_set_from_pixbuf(EinaCoverClutter *self, GdkPixbuf *pixbuf)
{
	g_warning("Not implemented");
	if (pixbuf)
		g_object_unref(pixbuf);
}

