/*
 * eina/player/eina-cover.c
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

#include "eina-cover.h"
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include "eina-cover-default-cover.h"
#include "eina-cover-loading-cover.h"

G_DEFINE_TYPE (EinaCover, eina_cover, GTK_TYPE_VBOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER, EinaCoverPrivate))

// #define debug(...) g_warning(__VA_ARGS__)
#define debug(...) ;

typedef struct _EinaCoverPrivate EinaCoverPrivate;

struct _EinaCoverPrivate {
	LomoPlayer *lomo;      // <Extern object, used for monitor changes
	EinaArt    *art;       // <Extern object, used for search covers
	GtkWidget  *renderer;  // <Renderer

	GdkPixbuf *default_pb; // <Default pixbuf to use if no cover is found
	GdkPixbuf *loading_pb; // <Pixbuf for used while search is performed
	GdkPixbuf *curr_pb;    // <Pointer alias for current pixbuf

	EinaArtSearch *search; // <Search in progress

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

void
cover_set_pixbuf(EinaCover *self, GdkPixbuf *pb);
static void
weak_ref_cb (gpointer data, GObject *_object);
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

#define dispose_pixbuf(pb) \
	G_STMT_START { \
		if (pb) \
		{ \
			g_object_weak_unref((GObject *) pb, weak_ref_cb, NULL); \
			g_object_unref(pb); \
			pb = NULL; \
		} \
	} G_STMT_END

static void
eina_cover_dispose (GObject *object)
{
	EinaCover *self = EINA_COVER(object);
	EinaCoverPrivate *priv = GET_PRIVATE(self);

	dispose_pixbuf(priv->default_pb);
	dispose_pixbuf(priv->loading_pb);

	G_OBJECT_CLASS (eina_cover_parent_class)->dispose (object);
}

static void
eina_cover_add(GtkContainer *container, GtkWidget *widget)
{
	GList *l = gtk_container_get_children(container);
	g_return_if_fail(l == NULL);

	GTK_CONTAINER_CLASS(eina_cover_parent_class)->add(container, widget);
}

static void
eina_cover_get_preferred_x_for_x(GtkWidget *widget, gint i, gint *minimum, gint *natural)
{
	*minimum = *natural = i;
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
	widget_class->get_preferred_width_for_height = eina_cover_get_preferred_x_for_x;
	widget_class->get_preferred_height_for_width = eina_cover_get_preferred_x_for_x;

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
	static GdkPixbuf *default_pb = NULL, *loading_pb = NULL;
	GError *error = NULL;

	if (!default_pb)
		if (!(default_pb = gdk_pixbuf_new_from_inline(-1, __cover_default_png, FALSE, &error)))
		{
			g_warning(N_("Unable to load embed default cover: %s"), error->message);
			g_error_free(error);
			error = NULL;
		}

	if (!loading_pb)
		if (!(loading_pb = gdk_pixbuf_new_from_inline(-1, __cover_loading_png, FALSE, &error)))
		{
			g_warning(N_("Unable to load embed loading cover: %s"), error->message);
			g_error_free(error);
			error = NULL;
		}

	eina_cover_set_default_pixbuf(self, default_pb);
	eina_cover_set_loading_pixbuf(self, loading_pb);
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
		gtk_container_remove((GtkContainer *) self, priv->renderer); 

	priv->renderer = renderer;
	if (priv->renderer)
	{
		gtk_container_add(GTK_CONTAINER(self), renderer);
		gtk_widget_set_visible(renderer, TRUE);

		if (priv->curr_pb || priv->default_pb)
			cover_set_pixbuf(self,  priv->curr_pb ? priv->curr_pb : priv->default_pb );
	}
	else
	{
		g_warning("Renderer is NULL");
	}

	g_object_notify((GObject *) self, "renderer");
}

GtkWidget*
eina_cover_get_renderer(EinaCover *self)
{
	g_return_val_if_fail(EINA_IS_COVER(self), NULL);
	return GET_PRIVATE(self)->renderer;
}

void
eina_cover_set_art(EinaCover *self, EinaArt *art)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);

	if (priv->search)
		g_object_unref(priv->art);
	priv->art = art;
	g_object_notify((GObject *) self, "art");
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
	g_object_notify((GObject *) self, "lomo-player");
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

	if (pixbuf)
	{
		priv->default_pb = g_object_ref_sink(pixbuf);
		g_object_weak_ref((GObject *) priv->default_pb, (GWeakNotify) weak_ref_cb, NULL);
	}
	g_object_notify((GObject *) self, "default-pixbuf");
}

void
eina_cover_set_loading_pixbuf(EinaCover *self, GdkPixbuf *pixbuf)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	if (priv->loading_pb)
		g_object_unref(priv->loading_pb);

	if (pixbuf)
	{
		priv->loading_pb = g_object_ref_sink(pixbuf);
		g_object_weak_ref((GObject *) priv->loading_pb, (GWeakNotify) weak_ref_cb, NULL);
	}
	g_object_notify((GObject *) self, "loading-pixbuf");
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

	if ((pb == NULL) || !GDK_IS_PIXBUF(pb))
	{
		debug("Setting pixbuf to NULL (no search result?), using default");
		pb = priv->default_pb;
	}
	g_return_if_fail(GDK_IS_PIXBUF(pb));

	if (pb == priv->curr_pb)
		return;

	priv->curr_pb = pb;
	gboolean asis = (pb == priv->default_pb) || (pb == priv->loading_pb);
	priv->got_cover = !asis;
	debug("* Setting cover to %p (got_cover: %s)\n", pb, priv->got_cover ? "true" : "false");

	// Keep two call to g_object_set, renderer may not support asis property
	g_object_set((GObject *) priv->renderer, "as-is",  asis, NULL);
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
search_finish_cb(EinaArtSearch *search, EinaCover *self)
{
	EinaCoverPrivate *priv = GET_PRIVATE(self);
	priv->search = NULL;

	GdkPixbuf *pb = (GdkPixbuf *) eina_art_search_get_result(search);
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
		eina_art_cancel(priv->art, priv->search);
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
		debug(" no stream, set default cover\n");
		cover_set_pixbuf(self, priv->default_pb);
		return;
	}

	priv->search = eina_art_search(priv->art, stream, (EinaArtSearchCallback) search_finish_cb, self);
	debug(" new search started: %p\n", priv->search);
	if (priv->search)
	{
		debug(" set loading cover\n");
		cover_set_loading(self);
	}
}

static void
weak_ref_cb (gpointer data, GObject *_object)
{
	g_warning(N_("Protected object %p is begin destroyed. There is a bug somewhere, set a breakpoint on %s"), _object, __FUNCTION__);
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

