/*
 * plugins/fieshta/fieshta-stream.c
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

#include "fieshta-stream.h"
#include <glib/gprintf.h>
#include <clutter-gtk/clutter-gtk.h>
G_DEFINE_TYPE (FieshtaStream, fieshta_stream, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamPrivate))

typedef struct _FieshtaStreamPrivate FieshtaStreamPrivate;

struct _FieshtaStreamPrivate {
	GdkPixbuf *new;
};

static void
fieshta_stream_dispose (GObject *object)
{
  G_OBJECT_CLASS (fieshta_stream_parent_class)->dispose (object);
}

static void
fieshta_stream_class_init (FieshtaStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (FieshtaStreamPrivate));

  object_class->dispose = fieshta_stream_dispose;
}

static void
fieshta_stream_init (FieshtaStream *self)
{
}

FieshtaStream*
fieshta_stream_new (GdkPixbuf *cover, gchar *title, gchar *artist)
{
	FieshtaStream *self = g_object_new (FIESHTA_STREAM_TYPE_FIESHTA, NULL);
	ClutterColor white = {255, 255, 255, 255};

	self->cover =  gtk_clutter_texture_new_from_pixbuf(cover);
	clutter_actor_set_anchor_point_from_gravity(self->cover, CLUTTER_GRAVITY_CENTER);
	clutter_actor_set_size(self->cover, 256, 256);
	clutter_actor_set_position(self->cover, 128, 128);

	self->title = clutter_text_new_with_text("Sans Bold 70", title);
	clutter_actor_set_position(self->title, 256, 0);
	clutter_text_set_color((ClutterText *) self->title, &white);
	clutter_text_set_ellipsize((ClutterText *) self->title, TRUE);

	self->artist = clutter_text_new_with_text("Sans Bold 70", artist);
	clutter_actor_set_position(self->artist, 256, clutter_actor_get_height(self->title));
	clutter_text_set_color((ClutterText *) self->artist, &white);
	clutter_text_set_ellipsize((ClutterText *) self->artist, TRUE);

	clutter_container_add((ClutterContainer *) self, self->cover, self->title, self->artist, NULL);
 	clutter_actor_set_anchor_point((ClutterActor*) self, 256, 128);

	return self;
}

static void
timeline_marker_reached(ClutterTimeline *timeline, gchar *marker_name, gint  frame_num, FieshtaStream *self)
{
	FieshtaStreamPrivate *priv = GET_PRIVATE(self);
	if (!priv->new)
		return;
	gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) self->cover, priv->new, NULL);
	g_object_unref(priv->new);
	priv->new = NULL;
}

void
fieshta_stream_set_cover_from_pixbuf(FieshtaStream* self, GdkPixbuf *cover)
{
	FieshtaStreamPrivate *priv = GET_PRIVATE(self);
	if (priv->new)
		g_object_unref(priv->new);
	priv->new = gdk_pixbuf_flip(cover, TRUE);

	ClutterTimeline *timeline = clutter_timeline_new(500);
	clutter_timeline_add_marker_at_time(timeline, "half", 250);
	g_signal_connect((GObject *) timeline, "marker-reached", (GCallback) timeline_marker_reached, self);

	clutter_actor_animate_with_timeline(self->cover, CLUTTER_LINEAR, timeline, NULL, NULL);
	/*
	ClutterAlpha *alpha = clutter_alpha_new();
	clutter_alpha_set_timeline(alpha, timeline);
	clutter_alpha_set_func(alpha, CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

	ClutterBehaviour *r = clutter_behaviour_rotate_new(alpha, CLUTTER_Y_AXIS, CLUTTER_ROTATE_CW, 0, 180);
	clutter_behaviour_apply(r, self->cover);
	
	clutter_timeline_start(timeline);
	*/
}
