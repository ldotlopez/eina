/*
 * eina/player/eina-cover-image.c
 *
 * Copyright (C) 2004-2010 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "eina-cover-image.h"
#include "eina-cover-image-mask.h"
#include <glib/gi18n.h>
#include <gel/gel.h>

G_DEFINE_TYPE (EinaCoverImage, eina_cover_image, GTK_TYPE_DRAWING_AREA)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_IMAGE, EinaCoverImagePrivate))

typedef struct _EinaCoverImagePrivate EinaCoverImagePrivate;

struct _EinaCoverImagePrivate {
	GdkPixbuf *pixbuf, *mask;
	GtkAllocation allocation;
	gint pb_w, pb_h, m_w, m_h;
	gfloat scale;
	cairo_t *cr;
};

enum {
	PROPERTY_COVER = 1,
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
	EinaCoverImage *self = EINA_COVER_IMAGE(object);
	EinaCoverImagePrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->pixbuf, NULL, g_object_unref);

	G_OBJECT_CLASS (eina_cover_image_parent_class)->dispose (object);
}

static gboolean
eina_cover_image_draw(GtkWidget *widget, cairo_t *_cr)
{
	EinaCoverImage        *self = EINA_COVER_IMAGE(widget);
	EinaCoverImagePrivate *priv = GET_PRIVATE(self);
	if (!priv->pixbuf)
		return TRUE;

	cairo_scale(_cr, priv->allocation.width / (gfloat) priv->pb_w, priv->allocation.height / (gfloat) priv->pb_h);
	gdk_cairo_set_source_pixbuf(_cr, priv->pixbuf, 0, 0);
	cairo_paint(_cr);

	if (priv->mask)
	{
		// Create mask
		cairo_push_group(_cr);
		cairo_identity_matrix(_cr);
		cairo_scale(_cr,
			priv->allocation.width  / (gfloat) priv->m_w,
			priv->allocation.height / (gfloat) priv->m_h);
		gdk_cairo_set_source_pixbuf(_cr, priv->mask, 0, 0);
		cairo_paint(_cr);
		cairo_pattern_t *mask = cairo_pop_group(_cr);

		// Create new combined pattern
		cairo_push_group(_cr);

		GdkRGBA color;
		gtk_style_context_get_background_color(
			gtk_widget_get_style_context(GTK_WIDGET(gtk_widget_get_toplevel(widget))),
			GTK_STATE_FLAG_NORMAL,
			&color);

		cairo_set_source_rgba(_cr, color.red, color.green, color.blue, color.alpha);
		cairo_mask(_cr, mask);
		cairo_pattern_t *f = cairo_pop_group(_cr);
		cairo_pattern_destroy(mask);

		// Apply all
		cairo_set_source(_cr, f);
		cairo_pattern_destroy(f);

		cairo_paint(_cr);
	}

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

	widget_class->draw = eina_cover_image_draw;
	widget_class->configure_event = eina_cover_image_configure_event;

    g_object_class_install_property(object_class, PROPERTY_COVER,
		g_param_spec_object("cover", "Cover", "Cover pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_cover_image_init (EinaCoverImage *self)
{
	EinaCoverImagePrivate *priv = GET_PRIVATE(self);
	if (priv->mask == NULL)
	{
		GError *err = NULL;
		if ((priv->mask = gdk_pixbuf_new_from_inline(-1, __cover_mask_png, FALSE, &err)) == NULL)
		{
			g_warning(N_("Unable to load cover mask: %s"), err->message);
			priv->m_w = priv->m_h = -1;
			g_error_free(err);
			return;
		}
		priv->m_w = gdk_pixbuf_get_width (priv->mask);
		priv->m_h = gdk_pixbuf_get_height(priv->mask);
	}
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

	priv->pixbuf = pixbuf ? g_object_ref(pixbuf) : NULL;
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

	g_object_notify((GObject *) self, "cover");
}

