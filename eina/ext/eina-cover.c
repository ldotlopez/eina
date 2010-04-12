/*
 * eina/ext/eina-cover.c
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

#include <glib/gprintf.h>
#include "eina-cover.h"

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_VBOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER, EinaCoverPrivate))

// #define debug(...) g_printf(__VA_ARGS__)
#define debug(...) ;

typedef struct _EinaCoverPrivate EinaCoverPrivate;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;      // <EXtern object, used for monitor changes
	Art        *art;       // <Extern object, used for search covers
	GtkWidget  *renderer;  // <Renderer

	GdkPixbuf *default_pb; // <Default pixbuf to use if no cover is found
	GdkPixbuf *loading_pb; // <Pixbuf for used while search is performed
	GdkPixbuf *curr_pb;    // <Pointer alias for current pixbuf

	ArtSearch *search;     // <Search in progress

	guint loading_timeout; // <Used to draw a loading cover after a timeout
	gboolean got_cover;    // <Flag to indicate if cover was found
};

enum {
	PROPERTY_RENDERER = 1,
	PROPERTY_LOMO_PLAYER,
	PROPERTY_ART,

	PROPERTY_DEFAULT_PIXBUF,
	PROPERTY_LOADING_PIXBUF
};

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self);
static void
lomo_clear_cb(LomoPlayer *lomo,  EinaCover *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream , EinaCover *self);

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
	case PROPERTY_LOMO_PLAYER:
		eina_cover_set_lomo_player((EinaCover *) object, g_value_get_object(value));
		break;
	case PROPERTY_ART:
		eina_cover_set_art((EinaCover *) object, g_value_get_pointer(value));
		break;
	case PROPERTY_DEFAULT_PIXBUF:
		eina_cover_set_default_pixbuf((EinaCover *) object, g_value_get_object(value));
		break;
	case PROPERTY_LOADING_PIXBUF:
		eina_cover_set_loading_pixbuf((EinaCover *) object, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_cover_parent_class)->dispose (object);
}

static void
eina_cover_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	requisition->width  = 0;
	requisition->height = 0;
	// debug("EinaCover size request to %dx%d\n", requisition->width, requisition->height);
}

static void
eina_cover_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GList *l = gtk_container_get_children(GTK_CONTAINER(widget));
	GtkWidget *child = l ? l->data : NULL;
	g_list_free(l);

	if (child && gtk_widget_get_visible(child))
	{
		// Make an square!
		allocation->width = allocation->height;
		gtk_widget_set_size_request(widget, allocation->height, allocation->height);
		gtk_widget_size_allocate(child, allocation);
		// debug("EinaCover size allcate to  %dx%d\n", allocation->width, allocation->height);
	}
}

static void
eina_cover_add(GtkContainer *container, GtkWidget *widget)
{
	GList *l = gtk_container_get_children(container);
	g_return_if_fail(l == NULL);

	GTK_CONTAINER_CLASS(eina_cover_parent_class)->add(container, widget);
}

static void
eina_cover_class_init (EinaCoverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS(klass);

	g_type_class_add_private (klass, sizeof (EinaCoverPrivate));

	object_class->get_property = eina_cover_get_property;
	object_class->set_property = eina_cover_set_property;
	object_class->dispose = eina_cover_dispose;
	widget_class->size_request = eina_cover_size_request;
	widget_class->size_allocate = eina_cover_size_allocate;
	container_class->add = eina_cover_add;

	g_object_class_install_property(object_class, PROPERTY_RENDERER,
		g_param_spec_object("renderer", "Renderer object", "Widget used to render images",
		GTK_TYPE_WIDGET, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROPERTY_ART,
		g_param_spec_pointer("art", "Art interface", "Art interface",
		G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROPERTY_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "Lomo player", "Lomo Player to control/watch",
		LOMO_TYPE_PLAYER, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROPERTY_DEFAULT_PIXBUF,
		g_param_spec_object("default-pixbuf", "Default pixbuf", "Default pixbuf to display",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROPERTY_LOADING_PIXBUF,
		g_param_spec_object("loading-pixbuf", "Loading pixbuf", "Loading pixbuf to display",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_cover_init (EinaCover *self)
{
}

EinaCover*
eina_cover_new (void)
{
	return g_object_new (EINA_TYPE_COVER, NULL);
}

void
eina_cover_set_renderer(EinaCover *self, GtkWidget *renderer)
{
	g_return_if_fail(EINA_IS_COVER(self));
	if (renderer)
		g_return_if_fail(GTK_IS_WIDGET(self));

	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->renderer)
	{
		gtk_container_remove((GtkContainer *) self, priv->renderer); 
		// g_object_unref(priv->renderer);
	}

	priv->renderer = renderer;
	if (priv->renderer)
	{
		gtk_container_add(GTK_CONTAINER(self), renderer);
		gtk_widget_set_visible(renderer, TRUE);
		// g_object_ref(renderer);

		g_object_set(renderer, "asis",   ((priv->curr_pb == priv->default_pb) || (priv->curr_pb == priv->loading_pb)), NULL);
		g_object_set(renderer, "cover", priv->curr_pb ? priv->curr_pb : priv->default_pb, NULL);
	}
	else
	{
		g_warning("Renderer is NULL");
	}
}

GtkWidget*
eina_cover_get_renderer(EinaCover *self)
{
	g_return_val_if_fail(EINA_IS_COVER(self), NULL);
	return GET_PRIVATE(self)->renderer;
}

void
eina_cover_set_art(EinaCover *self, Art *art)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (priv->search)
		art_cancel(priv->art, priv->search);
	priv->art = art;
}

void
eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_COVER(self));
	if (lomo)
		g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->lomo)
	{
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_change_cb,   self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_clear_cb,    self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_all_tags_cb, self);
	}
	priv->lomo = lomo;
	if (!lomo)
		return;

	g_signal_connect(priv->lomo, "change",   (GCallback) lomo_change_cb,   self);
	g_signal_connect(priv->lomo, "clear",    (GCallback) lomo_clear_cb,    self);
	g_signal_connect(priv->lomo, "all-tags", (GCallback) lomo_all_tags_cb, self);
}

LomoPlayer*
eina_cover_get_lomo_player(EinaCover *self)
{
	return GET_PRIVATE(self)->lomo;
}

void
eina_cover_set_default_pixbuf(EinaCover *self, GdkPixbuf *pixbuf)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->default_pb)
		g_object_unref(priv->default_pb);

	priv->default_pb = pixbuf;
	if (pixbuf)
		g_object_ref(pixbuf);
}

void
eina_cover_set_loading_pixbuf(EinaCover *self, GdkPixbuf *pixbuf)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->loading_pb)
		g_object_unref(priv->loading_pb);

	priv->loading_pb = pixbuf;
	if (pixbuf)
		g_object_ref(pixbuf);
}

void
cover_set_pixbuf(EinaCover *self, GdkPixbuf *pb)
{
	g_return_if_fail(EINA_IS_COVER(self));
	EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (priv->loading_timeout)
	{
		g_source_remove(priv->loading_timeout);
		priv->loading_timeout = 0;
	}

	if (!priv->renderer)
		return;

	if (pb == NULL)
		pb = priv->default_pb;
	g_return_if_fail(GDK_IS_PIXBUF(pb));

	if (pb == priv->curr_pb)
		return;

	priv->curr_pb = pb;
	gboolean asis = (pb == priv->default_pb) || (pb == priv->loading_pb);
	priv->got_cover = !asis;
	debug("* Setting cover to %p (got_cover: %s)\n", pb, priv->got_cover ? "true" : "false");

	// Keep two call to g_object_set, renderer may not support asis property
	g_object_set((GObject *) priv->renderer, "asis",  asis, NULL);
	g_object_set((GObject *) priv->renderer, "cover", asis ? gdk_pixbuf_copy(pb) : pb, NULL);
}

static gboolean
_cover_set_loading_real(EinaCover *self)
{
	g_return_val_if_fail(EINA_IS_COVER(self), FALSE);
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->loading_timeout = 0;

	cover_set_pixbuf(self, priv->loading_pb);

	return FALSE;
}

static void
cover_set_loading(EinaCover *self)
{
	g_return_if_fail(EINA_IS_COVER(self));
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->loading_timeout)
		g_source_remove(priv->loading_timeout);
	priv->loading_timeout = g_timeout_add(500, (GSourceFunc) _cover_set_loading_real, self);
}

static void
search_finish_cb(Art *art, ArtSearch *search, EinaCover *self)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->search = NULL;

	GdkPixbuf *pb = art_search_get_result(search);
	debug("* Search got result: %p\n", pb);
	cover_set_pixbuf(self, pb);
}

static void
cover_set_stream(EinaCover *self, LomoStream *stream)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->got_cover = FALSE;

	debug("* Got new stream %p\n", stream);
	if (priv->art && priv->search)
	{
		debug(" discart running search %p\n", priv->search);
		art_cancel(priv->art, priv->search);
		priv->search = NULL;
	}
	if (!priv->renderer)
	{
		debug(" no renderer, no search\n");
		return;
	}
	if (!priv->art)
	{
		debug(" no art interface, set default cover\n");
		cover_set_pixbuf(self, priv->default_pb);
		return;
	}

	if (!stream)
	{
		debug(" no stream\n");
		cover_set_pixbuf(self, priv->default_pb);
		return;
	}

	priv->search = art_search(priv->art, stream, (ArtFunc) search_finish_cb, self);
	debug(" new search started: %p\n", priv->search);
	if (priv->search)
	{
		debug(" set loading cover\n");
		cover_set_loading(self);
	}
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	cover_set_stream(self, lomo_player_get_current_stream(priv->lomo));
}

static void
lomo_clear_cb(LomoPlayer *lomo,  EinaCover *self)
{
	cover_set_stream(self, NULL);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream , EinaCover *self)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->got_cover || (stream != lomo_player_get_current_stream(priv->lomo)))
		return;
	cover_set_stream(self, stream);
}

