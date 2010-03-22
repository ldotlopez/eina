#include "eina-cover-image.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaCoverImage, eina_cover_image, GTK_TYPE_DRAWING_AREA)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_IMAGE, EinaCoverImagePrivate))

typedef struct _EinaCoverImagePrivate EinaCoverImagePrivate;

struct _EinaCoverImagePrivate {
	GdkPixbuf *pixbuf;
	GdkPixbuf *scaled;
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

/*
static void
eina_cover_image_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	g_printf("Need to resize to %dx%d\n", allocation->width, allocation->height);
	GTK_WIDGET_CLASS(eina_cover_image_parent_class)->size_allocate(widget, allocation);

	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));
	if (!priv->pixbuf)
		return;

	gfloat w, h;
	w = (gfloat) gdk_pixbuf_get_width(priv->pixbuf);
	h = (gfloat) gdk_pixbuf_get_height(priv->pixbuf);

	gfloat scale = MIN(
		(gfloat) allocation->width  / w,
		(gfloat) allocation->height / h);

	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(priv->pixbuf,
		w * scale,
		h * scale,
		GDK_INTERP_HYPER);
	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled);
}
*/

static gboolean
eina_cover_image_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));
	if (priv->scaled == NULL)
		return TRUE;

	gdk_draw_pixbuf((GdkDrawable *) gtk_widget_get_window(widget),
		NULL,
		priv->scaled,
		ev->area.x, ev->area.y,
		ev->area.x, ev->area.y,
		ev->area.width, ev->area.height,
		GDK_RGB_DITHER_NONE,
		0, 0);
	g_printf("Got expose\n");
	return TRUE;
}

static gboolean
eina_cover_image_configure_event(GtkWidget *widget, GdkEventConfigure *ev)
{
	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));

	if (priv->pixbuf == NULL)
		return TRUE;
	if (priv->scaled != NULL)
		g_object_unref(priv->scaled);

	gfloat w, h;
	w = (gfloat) gdk_pixbuf_get_width(priv->pixbuf);
	h = (gfloat) gdk_pixbuf_get_height(priv->pixbuf);

	gfloat scale = MIN(
		(gfloat) ev->width  / w,
		(gfloat) ev->height / h);

	priv->scaled = gdk_pixbuf_scale_simple(priv->pixbuf,
		w * scale,
		h * scale,
		GDK_INTERP_HYPER);
	g_printf("Configure done (%dx%d)", (int) (w*scale), (int) (h*scale));
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
	g_object_ref(priv->pixbuf);
}

