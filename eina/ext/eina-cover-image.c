#include "eina-cover-image.h"

G_DEFINE_TYPE (EinaCoverImage, eina_cover_image, GTK_TYPE_DRAWING_AREA)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_IMAGE, EinaCoverImagePrivate))

typedef struct _EinaCoverImagePrivate EinaCoverImagePrivate;

struct _EinaCoverImagePrivate {
	GdkPixbuf *pixbuf;
	GtkAllocation allocation;
	gint pb_w, pb_h;
	gfloat scale;
	cairo_t *cr;
};

enum {
	PROPERTY_COVER = 1
};

static void
eina_cover_image_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_image_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_COVER:
		eina_cover_image_set_from_pixbuf((EinaCoverImage *) object, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_image_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_cover_image_parent_class)->dispose (object);
}

static gboolean
eina_cover_image_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));
	if (!priv->pixbuf)
		return TRUE;

	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_scale(cr, priv->scale, priv->scale);
	gdk_cairo_set_source_pixbuf(cr, priv->pixbuf, 0, 0);

	GdkRectangle *rects = NULL;
	gint n_rects;
	gdk_region_get_rectangles(ev->region, &rects, &n_rects);

	gint i;
	for (i = 0; i < n_rects; i++)
	{
		cairo_rectangle(cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cairo_paint(cr);
	}
	g_free(rects);
	cairo_destroy(cr);

	return TRUE;
}

static gboolean
eina_cover_image_configure_event(GtkWidget *widget, GdkEventConfigure *ev)
{
	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));
	gtk_widget_get_allocation(widget, &priv->allocation);
	priv->scale = MAX(priv->allocation.width / (gfloat) priv->pb_w, priv->allocation.height / (gfloat) priv->pb_h);
	gtk_widget_queue_draw(widget);
	return TRUE;
}

static void
eina_cover_image_class_init (EinaCoverImageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCoverImagePrivate));

	object_class->get_property = eina_cover_image_get_property;
	object_class->set_property = eina_cover_image_set_property;
	object_class->dispose = eina_cover_image_dispose;

	widget_class->expose_event = eina_cover_image_expose_event;
	widget_class->configure_event = eina_cover_image_configure_event;

    g_object_class_install_property(object_class, PROPERTY_COVER,
		g_param_spec_object("cover", "Cover", "Cover pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_cover_image_init (EinaCoverImage *self)
{
}

EinaCoverImage*
eina_cover_image_new (void)
{
	return g_object_new (EINA_TYPE_COVER_IMAGE, NULL);
}

void
eina_cover_image_set_from_pixbuf(EinaCoverImage *self, GdkPixbuf *pixbuf)
{
	g_return_if_fail(EINA_IS_COVER_IMAGE(self) || GDK_IS_PIXBUF(pixbuf));
	EinaCoverImagePrivate *priv = GET_PRIVATE(self);
	if (priv->pixbuf)
		g_object_unref(priv->pixbuf);

	priv->pixbuf = pixbuf;
	if (!priv->pixbuf)
	{
		priv->pb_w = priv->pb_h = -1;
		priv->scale = 0;
		return;
	}

	priv->pb_w = gdk_pixbuf_get_width(priv->pixbuf);
	priv->pb_h = gdk_pixbuf_get_width(priv->pixbuf);
	priv->scale = MAX(priv->allocation.width / (gfloat) priv->pb_w, priv->allocation.height / (gfloat) priv->pb_h);
	gtk_widget_queue_draw(GTK_WIDGET(self));
}

