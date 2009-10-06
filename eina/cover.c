/*
 * eina/cover.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::Cover"

// Stds
#include <config.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <gmodule.h>

// Modules
#include <eina/eina-plugin.h>
#include <eina/player.h>

#if HAVE_CLUTTER
#include <eina/ext/eina-clutty.h>
#define clutty_enabled(self) (TRUE)
#else
#define clutty_enabled(self) (FALSE)
#endif

#define search_cancel(self) \
	G_STMT_START {                                            \
		if (self->search)                                     \
		{                                                     \
			art_cancel(EINA_OBJ_GET_ART(self), self->search); \
			self->search = NULL;                              \
			self->loading = FALSE;                            \
		}                                                     \
	} G_STMT_END

#define cover_height(self) (((GtkWidget *)self->cover)->parent->allocation.height)

typedef struct _EinaCover {
	EinaObj    parent;

	GtkWidget *cover;
	GdkPixbuf *cover_default;
	GdkPixbuf *cover_loading;
	GdkPixbuf *cover_mask;

	ArtSearch *search;

	gboolean   got_cover;
	gboolean   loading;
} EinaCover;

static GdkPixbuf*
build_cover_mask(EinaCover *self);

static void
cover_update_from_pixbuf(EinaCover *self, GdkPixbuf *pixbuf);
static void
cover_update_from_stream(EinaCover *self, LomoStream *stream);

static void
art_search_cb(Art *art, ArtSearch *search, EinaCover *self);

static gboolean
cover_expose_event_idle_cb(EinaCover *self);

gboolean cover_expose_event_cb
(GtkWidget *w, GdkEventExpose *ev, EinaCover *self);
static void lomo_change_cb
(LomoPlayer *lomo, gint from, gint to, EinaCover *self);
static void lomo_clear_cb
(LomoPlayer *lomo, EinaCover *self);
static void lomo_all_tags_cb
(LomoPlayer *lomo, LomoStream *stream, EinaCover *self);


static gboolean
cover_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaCover *self = NULL;

	// Initialize base class
	self = g_new0(EinaCover, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "cover", EINA_OBJ_NONE, error))
	{
		g_free(self);
		return FALSE;
	}

	#if HAVE_CLUTTER
	if (clutty_enabled(self))
	{
		gtk_clutter_init(NULL, NULL);
		self->cover = (GtkWidget*) eina_clutty_new();
	}
	else
		self->cover        = gtk_image_new();
	#else
	self->cover        = gtk_image_new();
	#endif

	self->got_cover    = FALSE;
	self->loading      = FALSE;

	GtkContainer *cover_container = eina_player_get_cover_container(EINA_OBJ_GET_PLAYER(self));

	g_signal_connect(cover_container, "expose-event", G_CALLBACK(cover_expose_event_cb), self);

	return TRUE;
}

static gboolean
cover_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaCover *self = gel_app_shared_get(app, "cover");
	g_return_val_if_fail(self != NULL, FALSE);

	search_cancel(self);
	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

static gboolean
cover_reload_resources(EinaCover *self)
{
	gchar *cover_default = eina_obj_get_resource(self, GEL_RESOURCE_IMAGE, "cover-default.png");
	gchar *cover_loading = eina_obj_get_resource(self, GEL_RESOURCE_IMAGE, "cover-loading.png");
	if (!cover_default || ! cover_loading)
	{
		gel_warn("Cannot locate resource '%s'", cover_default ? "cover-loading.png" : "cover-default.png");
		gel_free_and_invalidate(cover_default, NULL, g_free);
		gel_free_and_invalidate(cover_loading, NULL, g_free);
		return FALSE;
	}

	GError *err = NULL;
	gel_free_and_invalidate(self->cover_default, NULL, g_object_unref);
	gel_free_and_invalidate(self->cover_loading, NULL, g_object_unref);
	self->cover_default  = gdk_pixbuf_new_from_file_at_scale(cover_default,
		cover_height(self), cover_height(self),
		FALSE, &err);
	self->cover_loading  = gdk_pixbuf_new_from_file_at_scale(cover_loading,
		cover_height(self), cover_height(self),
		FALSE, &err);
	if (!self->cover_default || !self->cover_loading)
	{
		gel_warn("Cannot load resource '%s': '%s'",
			self->cover_default ? cover_loading : cover_default,
			err->message);
		gel_free_and_invalidate(self->cover_default, NULL, g_object_unref);
		gel_free_and_invalidate(self->cover_loading, NULL, g_object_unref);
		g_free(cover_default);
		g_free(cover_loading);
		return FALSE;
	}
	g_free(cover_default);
	g_free(cover_loading);

	gel_free_and_invalidate(self->cover_mask, NULL, g_object_unref);
	if (!(self->cover_mask  = build_cover_mask(self)))
		return FALSE;

	return TRUE;
}

static gboolean
cover_expose_event_idle_cb(EinaCover *self)
{
	GtkContainer *w = eina_player_get_cover_container(EINA_OBJ_GET_PLAYER(self));
	gtk_container_foreach(w, (GtkCallback) gtk_widget_hide, NULL);

	gtk_container_add(w, (GtkWidget *) self->cover);
	gtk_widget_set_size_request(GTK_WIDGET(self->cover), ((GtkWidget *) w)->allocation.height, ((GtkWidget *) w)->allocation.height);
	#if HAVE_CLUTTER
	if (clutty_enabled(self))
		clutter_actor_set_size(CLUTTER_ACTOR(gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self->cover))), w->allocation.height, w->allocation.height);
	#endif
	gtk_widget_show((GtkWidget *) self->cover);

	cover_reload_resources(self);

	cover_update_from_stream(self, lomo_player_get_current_stream(EINA_OBJ_GET_LOMO(self)));

	g_signal_connect(eina_obj_get_lomo(self), "change",   G_CALLBACK(lomo_change_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "clear",    G_CALLBACK(lomo_clear_cb), self);
	g_signal_connect(eina_obj_get_lomo(self), "all-tags", G_CALLBACK(lomo_all_tags_cb), self);

	return FALSE;
}

gboolean cover_expose_event_cb
(GtkWidget *w, GdkEventExpose *ev, EinaCover *self) 
{
	g_signal_handlers_disconnect_by_func(w, cover_expose_event_cb, self);
	g_idle_add((GSourceFunc) cover_expose_event_idle_cb, self);
	return FALSE;
}

static void
cover_update_from_pixbuf(EinaCover *self, GdkPixbuf *pixbuf)
{
	self->got_cover = (pixbuf != NULL);
	if (!self->got_cover)
		pixbuf = self->cover_default;
	
	if (pixbuf == self->cover_default)
	{
		gel_warn("Set default cover");
		self->got_cover = FALSE;
		self->loading   = FALSE;
	}
	else if (pixbuf == self->cover_loading)
	{
		gel_warn("Set loading cover");
		self->got_cover = FALSE;
		self->loading   = TRUE;
	}
	else
	{
		gel_warn("Set own cover");
		self->got_cover = TRUE;
		self->loading   = FALSE;
	}

	g_return_if_fail(GDK_IS_PIXBUF(pixbuf));

	// Scale pixbuf if needed
	if ((pixbuf != self->cover_default) &&
		(pixbuf != self->cover_loading))
	{
		GdkPixbuf *tmp = gdk_pixbuf_scale_simple(pixbuf,
			cover_height(self), cover_height(self),
			GDK_INTERP_BILINEAR);
		g_object_unref(pixbuf);
		pixbuf = tmp;
		self->got_cover = TRUE;
	}

	// Composite over new cover
	if ((pixbuf != self->cover_default) &&
		(pixbuf != self->cover_loading) &&
		self->cover_mask                &&
		eina_conf_get_bool(EINA_OBJ_GET_SETTINGS(self), "/cover/mask_effect", TRUE))
	{
		gdk_pixbuf_composite(self->cover_mask, pixbuf,
			0, 0,
			cover_height(self), cover_height(self),
			0, 0,
			gdk_pixbuf_get_width(pixbuf) / (double) gdk_pixbuf_get_width(self->cover_mask),
			gdk_pixbuf_get_height(pixbuf) / (double) gdk_pixbuf_get_height(self->cover_mask), 
			GDK_INTERP_BILINEAR,
			255);
	}

	#if HAVE_CLUTTER
	if (clutty_enabled(self))
		eina_clutty_set_pixbuf((EinaClutty *) self->cover, pixbuf);
	else
		gtk_image_set_from_pixbuf((GtkImage *) self->cover, pixbuf);
	#else
	gtk_image_set_from_pixbuf((GtkImage *) self->cover, pixbuf);
	#endif
}

static void
cover_update_from_stream(EinaCover *self, LomoStream *stream)
{
	Art *art = EINA_OBJ_GET_ART(self);

	search_cancel(self);
	if (!stream || !art)
	{
		cover_update_from_pixbuf(self, NULL); // NULL means default
		return;
	}
	
	cover_update_from_pixbuf(self, self->cover_loading);
	self->search  = art_search(art, stream, (ArtFunc) art_search_cb, self);
}

static void
art_search_cb(Art *art, ArtSearch *search, EinaCover *self)
{
	self->search = NULL;
	GdkPixbuf *pixbuf = (GdkPixbuf *) art_search_get_result(search);

	if (!pixbuf || !GDK_IS_PIXBUF(pixbuf))
	{
		cover_update_from_pixbuf(self, NULL);
		return;
	}

	cover_update_from_pixbuf(self, pixbuf);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaCover *self)
{
	cover_update_from_stream(self, lomo_player_nth_stream(lomo, to));
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaCover *self) 
{
	cover_update_from_pixbuf(self, NULL);
}

static void
lomo_all_tags_cb (LomoPlayer *lomo, LomoStream *stream, EinaCover *self) 
{
	if (stream != lomo_player_get_stream(lomo))
		return;

	if (!self->got_cover)
	 	cover_update_from_stream(self, stream);
}

static GdkPixbuf*
build_cover_mask(EinaCover *self)
{
	gchar *maskpath = eina_obj_get_resource(self, GEL_RESOURCE_IMAGE, "cover-mask.png");
	if (!maskpath)
	{
		gel_warn(N_("Cannot locate resource '%s'"), "cover-mask.png");
		return NULL;
	}

	GError *err = NULL;
	GdkPixbuf *mask = gdk_pixbuf_new_from_file_at_scale(maskpath,
		cover_height(self), cover_height(self),
		FALSE, &err);
	if (!mask)
	{
		gel_warn(N_("Cannot load resource '%s': '%s'"), maskpath, err->message);
		g_error_free(err);
		g_free(maskpath);
		return NULL;
	}
	g_free(maskpath);

	int width, height, rowstride, n_channels, i, j;
	guchar *pixels, *p;
	n_channels = gdk_pixbuf_get_n_channels(mask);
	if ((gdk_pixbuf_get_colorspace (mask) != GDK_COLORSPACE_RGB) ||
		(gdk_pixbuf_get_bits_per_sample (mask) != 8)             ||
		(!gdk_pixbuf_get_has_alpha (mask))                       ||
		(n_channels != 4))
	{
		g_object_unref(mask);
		g_warning(N_("Invalid cover mask"));
		return NULL;
	}

	width     = gdk_pixbuf_get_width     (mask);
	height    = gdk_pixbuf_get_height    (mask);
	rowstride = gdk_pixbuf_get_rowstride (mask);
	pixels    = gdk_pixbuf_get_pixels    (mask);

	GdkColor *color = gtk_widget_get_style((GtkWidget *) self->cover)->bg;
	for ( i = 0; i < width; i++)
		for (j = 0; j < height; j++)
		{
			p = pixels + j * rowstride + i * n_channels;
			p[0] = color->red;
			p[1] = color->green;
			p[2] = color->blue;
		}
	return mask;
}

// --
// Connector 
// --
EINA_PLUGIN_SPEC(cover,
	NULL,	// version
	"player",	// deps
	NULL,		// author
	NULL,		// url
	N_("Cover plugin"),
	NULL,		
	NULL,

	cover_init, cover_fini
);
