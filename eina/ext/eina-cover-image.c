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
cover_image_rescale(EinaCoverImage *self);
static void
cover_image_redraw(EinaCoverImage *self);

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
	#if 0
	EinaCoverImagePrivate *priv = GET_PRIVATE(EINA_COVER_IMAGE(widget));
	if (priv->scaled == NULL)
		return TRUE;

	gdk_draw_pixbuf((GdkDrawable *) gtk_widget_get_window(widget),
		NULL,
		priv->scaled,
		0, 0,
		0, 0,
		-1, -1,
		/*
		ev->area.x, ev->area.y,
		ev->area.x, ev->area.y,
		ev->area.width, ev->area.height,
		*/
		GDK_RGB_DITHER_NONE,
		0, 0);
	g_printf("Got expose\n");
	#endif
	cover_image_redraw(EINA_COVER_IMAGE(widget));
	return TRUE;
}

static gboolean
eina_cover_image_configure_event(GtkWidget *widget, GdkEventConfigure *ev)
{
	cover_image_rescale(EINA_COVER_IMAGE(widget));
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
	if (priv->scaled)
	{
		g_object_unref(priv->scaled);
		priv->scaled = NULL;
	}

	priv->pixbuf = pixbuf;
	if (priv->pixbuf)
		g_object_ref(priv->pixbuf);
	cover_image_redraw(self);
}

static void
cover_image_rescale(EinaCoverImage *self)
{
	g_return_if_fail(EINA_IS_COVER_IMAGE(self));
    EinaCoverImagePrivate *priv = GET_PRIVATE(self); 

	if (priv->pixbuf == NULL)
		return;
	if (priv->scaled != NULL)
		g_object_unref(priv->scaled);

	gfloat w, h;
	w = (gfloat) gdk_pixbuf_get_width(priv->pixbuf);
	h = (gfloat) gdk_pixbuf_get_height(priv->pixbuf);

	GtkAllocation alloc;
	gtk_widget_get_allocation(GTK_WIDGET(self), &alloc);
	gfloat scale = MIN(
		(gfloat) alloc.width  / w,
		(gfloat) alloc.height / h);

	priv->scaled = gdk_pixbuf_scale_simple(priv->pixbuf,
		w * scale,
		h * scale,
		GDK_INTERP_HYPER);
	g_printf("Rescale done (%dx%d)\n", (int) (w*scale), (int) (h*scale));
}

static void
cover_image_redraw(EinaCoverImage *self)
{
	g_return_if_fail(EINA_IS_COVER_IMAGE(self));
	EinaCoverImagePrivate *priv = GET_PRIVATE(self);
	if (!priv->pixbuf)
		return;

	if (!priv->scaled)
		cover_image_rescale(self);

	gdk_draw_pixbuf((GdkDrawable *) gtk_widget_get_window(GTK_WIDGET(self)),
		NULL,
		priv->scaled,
		0, 0,
		0, 0,
		-1, -1,
		/*
		ev->area.x, ev->area.y,
		ev->area.x, ev->area.y,
		ev->area.width, ev->area.height,
		*/
		GDK_RGB_DITHER_NONE,
		0, 0);
	g_printf("Redraw done\n");
}

