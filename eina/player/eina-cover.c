/*
 * eina/player/eina-cover.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-cover.h"
#include <glib/gi18n.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_VBOX)

#define SIZE_HACKS 1

// #define debug(...) g_warning(__VA_ARGS__)
#define debug(...) ;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;      // <Extern object, used for monitor changes
	GtkWidget  *renderer;  // <Renderer
	GdkPixbuf  *default_pb;

	gboolean has_cover;

	LomoStream *stream;
	gulong      stream_em_handler;
};

enum {
	PROPERTY_DEFAULT_PIXBUF = 1,
	PROPERTY_RENDERER,
	PROPERTY_LOMO_PLAYER
};

static void
cover_set(EinaCover *self, const gchar *uri);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self);
static void
lomo_clear_cb(LomoPlayer *lomo,  EinaCover *self);

static void
eina_cover_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	EinaCover *self = (EinaCover *) object;

	switch (property_id) {
	case PROPERTY_RENDERER:
		g_value_set_object(value, (GObject *) eina_cover_get_renderer(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_RENDERER:
		eina_cover_set_renderer((EinaCover *) object, g_value_get_object(value));
		break;
	case PROPERTY_DEFAULT_PIXBUF:
		eina_cover_set_default_pixbuf((EinaCover *) object, g_value_get_object(value));
		break;
	case PROPERTY_LOMO_PLAYER:
		eina_cover_set_lomo_player((EinaCover *) object, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_dispose (GObject *object)
{
	EinaCover *self = EINA_COVER(object);
	EinaCoverPrivate *priv = self->priv;

	if (priv->stream_em_handler)
	{
		g_signal_handler_disconnect(priv->stream, priv->stream_em_handler);
		priv->stream_em_handler = 0;
	}

	if (priv->default_pb)
	{
		g_object_unref(priv->default_pb);
		priv->default_pb = NULL;
	}

	G_OBJECT_CLASS (eina_cover_parent_class)->dispose (object);
}

static void
eina_cover_container_add(GtkContainer *container, GtkWidget *widget)
{
	GList *l = gtk_container_get_children(container);
	g_return_if_fail(l == NULL);
	g_list_free(l);

	GTK_CONTAINER_CLASS(eina_cover_parent_class)->add(container, widget);
}

#if SIZE_HACKS
static void
eina_cover_get_preferred_h_for_w(GtkWidget *widget, gint i, gint *minimum, gint *natural)
{
	//g_debug("%s", __FUNCTION__);
	//g_debug("  Proposed ↑↓ %d", i);
	*minimum = *natural = i;
}

static void
eina_cover_get_preferred_w_for_h(GtkWidget *widget, gint i, gint *minimum, gint *natural)
{
	//g_debug("%s", __FUNCTION__);
	//g_debug("  Proposed ←→ %d", i);
	*minimum = *natural = i;
}

static GtkSizeRequestMode
eina_cover_get_request_mode(GtkWidget *widget)
{
	return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
eina_cover_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	g_warn_if_fail(allocation->width == allocation->height);

	// This is probabilly very incorrect
	gtk_widget_set_size_request(widget, allocation->width, allocation->height);

	GList *children = gtk_container_get_children((GtkContainer *) widget);
	GtkWidget *child = children ? (GtkWidget *) children->data : NULL;

	if (child && gtk_widget_get_visible(child))
		gtk_widget_size_allocate(child, allocation);
}
#endif

static void
eina_cover_class_init (EinaCoverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS(klass);
	#if SIZE_HACKS
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	#endif

	g_type_class_add_private (klass, sizeof (EinaCoverPrivate));

	object_class->get_property = eina_cover_get_property;
	object_class->set_property = eina_cover_set_property;
	object_class->dispose = eina_cover_dispose;
	container_class->add = eina_cover_container_add;

	#if SIZE_HACKS
	widget_class->size_allocate = eina_cover_size_allocate;
	widget_class->get_preferred_width_for_height = eina_cover_get_preferred_w_for_h;
	if (0) widget_class->get_preferred_height_for_width = eina_cover_get_preferred_h_for_w;
	widget_class->get_request_mode               = eina_cover_get_request_mode;
	#endif

	g_object_class_install_property(object_class, PROPERTY_RENDERER,
		g_param_spec_object("renderer", "Renderer object", "Widget used to render images",
		GTK_TYPE_WIDGET, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class, PROPERTY_DEFAULT_PIXBUF,
		g_param_spec_object("default-pixbuf", "Default pixbuf", "Default pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class, PROPERTY_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "Lomo player", "Lomo Player to control/watch",
		LOMO_TYPE_PLAYER,  G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}

static void
eina_cover_init (EinaCover *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_COVER, EinaCoverPrivate));
}

EinaCover*
eina_cover_new (LomoPlayer *lomo, GdkPixbuf *pixbuf, GtkWidget *renderer)
{
	return g_object_new (EINA_TYPE_COVER,
		"lomo-player",    lomo,
		"default-pixbuf", pixbuf,
		"renderer",       renderer,
		NULL);
}

void
eina_cover_set_renderer(EinaCover *self, GtkWidget *renderer)
{
	g_return_if_fail(EINA_IS_COVER(self));

	if (renderer)
		g_return_if_fail(GTK_IS_WIDGET(renderer));

	EinaCoverPrivate *priv = self->priv;

	// unset old object
	if (priv->renderer)
		gtk_container_remove((GtkContainer *) self, priv->renderer); 

	// setup new object if any
	priv->renderer = renderer;
	if (priv->renderer)
	{
		gtk_container_add(GTK_CONTAINER(self), renderer);
		gtk_widget_set_visible(renderer, TRUE);
		cover_set(self, NULL);
	}

	g_object_notify((GObject *) self, "renderer");
}

GtkWidget*
eina_cover_get_renderer(EinaCover *self)
{
	g_return_val_if_fail(EINA_IS_COVER(self), NULL);
	return self->priv->renderer;
}

void
eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_COVER(self));

	if (lomo)
		g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaCoverPrivate *priv = self->priv;

	// unset old object
	if (priv->lomo)
	{
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_change_cb,   self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_clear_cb,    self);
	}

	// Setup new object if any
	priv->lomo = lomo;
	if (priv->lomo)
	{
		g_signal_connect(priv->lomo, "change",   (GCallback) lomo_change_cb,   self);
		g_signal_connect(priv->lomo, "clear",    (GCallback) lomo_clear_cb,    self);
	}

	g_object_notify((GObject *) self, "lomo-player");
}

LomoPlayer*
eina_cover_get_lomo_player(EinaCover *self)
{
	g_return_val_if_fail(EINA_IS_COVER(self), NULL);
	return self->priv->lomo;
}

/**
 * eina_cover_set_default_pixbuf:
 * @self: An #EinaCover
 * @pixbuf: A #GdkPixbuf
 *
 * Sets the default pixbuf
 */
void
eina_cover_set_default_pixbuf(EinaCover *self, GdkPixbuf *pixbuf)
{
	g_return_if_fail(EINA_IS_COVER(self));
	g_return_if_fail(GDK_IS_PIXBUF(pixbuf));

	EinaCoverPrivate *priv = self->priv;
	g_return_if_fail(!priv->default_pb);

	priv->default_pb = pixbuf;

	GtkWidget *renderer = eina_cover_get_renderer(self);
	if (renderer && !priv->has_cover)
		cover_set(self, NULL);

	g_object_notify(G_OBJECT(self), "default-pixbuf");
}

static void
cover_set(EinaCover *self, const gchar *uri)
{
	g_return_if_fail(EINA_IS_COVER(self));
	EinaCoverPrivate *priv = self->priv;

	priv->has_cover = FALSE;
	if (uri == NULL)
	{
		g_object_set((GObject *) priv->renderer, "cover", priv->default_pb, NULL);
		return;
	}
	else
	{
		GFile *f = g_file_new_for_uri(uri);
		GError *error = NULL;
		GInputStream *input = G_INPUT_STREAM(g_file_read(f, NULL, &error));
		if (error)
		{
			g_warning("Unable to read uri '%s': '%s", uri, error->message);
			g_error_free(error);
			g_object_unref(f);
			return;
		}
		g_object_unref(f);

		GdkPixbuf *pb = gdk_pixbuf_new_from_stream(input, NULL, &error);
		if (error)
		{
			g_warning("Unable to load cover from uri '%s': '%s", uri, error->message);
			g_error_free(error);
			g_object_unref(input);
			return;
		}
		g_object_unref(input);
		g_object_set((GObject *) priv->renderer, "cover", pb, NULL);
		g_object_unref(pb);
		priv->has_cover = TRUE;
	}
}

static void
stream_em_updated(LomoStream *stream, const gchar *key, EinaCover *self)
{
	if (g_str_equal(key, "art-uri"))
		cover_set(self, (const gchar *) lomo_stream_get_extended_metadata(stream, key));
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	g_warn_if_fail(to >= 0);

	EinaCoverPrivate *priv = self->priv;
	if (priv->stream_em_handler)
	{
		g_signal_handler_disconnect(priv->stream, priv->stream_em_handler);
		priv->stream_em_handler = 0;
	}

	priv->stream = lomo_player_get_nth_stream(lomo, to);
	if (priv->stream == NULL);
	{
		cover_set(self, NULL);
		return;
	}

	gpointer d = lomo_stream_get_extended_metadata(priv->stream, "art-uri");
	if (d)
		cover_set(self, d);
	priv->stream_em_handler = g_signal_connect(priv->stream, "extended-metadata-updated", (GCallback) stream_em_updated, self);
}

static void
lomo_clear_cb(LomoPlayer *lomo,  EinaCover *self)
{
	EinaCoverPrivate *priv = self->priv;
	if (LOMO_IS_STREAM(priv->stream) && priv->stream_em_handler)
	{
		g_signal_handler_disconnect(priv->stream, priv->stream_em_handler);
		priv->stream_em_handler = 0;
	}
	cover_set(self, NULL);
}

