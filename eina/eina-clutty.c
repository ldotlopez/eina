/*
 * eina/eina-clutty.c
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

#include "eina-clutty.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaClutty, eina_clutty, GTK_TYPE_CLUTTER_EMBED)

#define CLUTTY_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_CLUTTY, EinaCluttyPrivate))

typedef struct _EinaCluttyPrivate EinaCluttyPrivate;

struct _EinaCluttyPrivate {
	ClutterActor *actor;
	ClutterTimeline *timeline;
	GdkPixbuf *old, *new;
	gboolean done;
};

void timeline_start(EinaClutty *self);
void timeline_stop(EinaClutty *self);
void timeline_new_frame_cb (ClutterTimeline *timeline, gint frame_num, EinaClutty *self);

static void
eina_clutty_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_clutty_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_clutty_dispose (GObject *object)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(object);

	timeline_stop((EinaClutty *) object);

	if (priv->old)
	{
		g_object_unref(priv->old);
		priv->old = NULL;
	}

	if (priv->new)
	{
		g_object_unref(priv->new);
		priv->new = NULL;
	}

	G_OBJECT_CLASS (eina_clutty_parent_class)->dispose (object);
}

static void
eina_clutty_class_init (EinaCluttyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCluttyPrivate));

	object_class->get_property = eina_clutty_get_property;
	object_class->set_property = eina_clutty_set_property;
	object_class->dispose = eina_clutty_dispose;
}

static void
eina_clutty_init (EinaClutty *self)
{
}

EinaClutty*
eina_clutty_new (void)
{	
	EinaClutty *self =  g_object_new (EINA_TYPE_CLUTTY, NULL);
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);

	priv->actor = clutter_texture_new();

	clutter_container_add_actor((ClutterContainer*) gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self)), priv->actor);

	priv->timeline = clutter_timeline_new_for_duration(1000);
	clutter_timeline_add_marker_at_frame (priv->timeline, "clutter-tutorial", 5);
	g_signal_connect (priv->timeline, "new-frame", G_CALLBACK(timeline_new_frame_cb), self);
	clutter_timeline_set_loop(priv->timeline, FALSE); 

	return self;
}

void
eina_clutty_set_from_pixbuf(EinaClutty *self, GdkPixbuf *pixbuf)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf,
		clutter_actor_get_width(gtk_clutter_embed_get_stage((GtkClutterEmbed *) self)),
		clutter_actor_get_height(gtk_clutter_embed_get_stage((GtkClutterEmbed *) self)),
		GDK_INTERP_BILINEAR);
	g_object_unref(pixbuf);

	// Set directly on stage
	if (!priv->old)
	{
		priv->old = scaled;
		gtk_clutter_texture_set_from_pixbuf((ClutterTexture*) priv->actor, priv->old);
		clutter_actor_set_anchor_point(priv->actor,
			clutter_actor_get_width(priv->actor) / 2,
			clutter_actor_get_height(priv->actor) / 2);
		ClutterActor *stage = (ClutterActor *) gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self));
		clutter_actor_set_position(priv->actor,
			clutter_actor_get_width(stage) / 2,
			clutter_actor_get_height(stage) / 2);
		return;
	}

	// Drop old new if any, hold new, start pipeline
	if (priv->new)
		g_object_unref(priv->new);
	priv->new = scaled;

	timeline_start(self);
}

//
// Timeline handling
//
void
timeline_start(EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);

	if (clutter_timeline_is_playing(priv->timeline))
		return;

	priv->done = FALSE;
	clutter_timeline_rewind(priv->timeline);
	clutter_timeline_start(priv->timeline);
}

void
timeline_stop(EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	if (!clutter_timeline_is_playing(priv->timeline))
		return;

	priv->done = FALSE;
	clutter_timeline_stop(priv->timeline);
}

void
timeline_new_frame_cb (ClutterTimeline *timeline, gint frame_num, EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	gint degrees = (180 / clutter_timeline_get_speed(timeline)) * frame_num * (1000/clutter_timeline_get_duration(timeline));

	g_printf("frame, degrees: %d\n", degrees);
	gdouble zoom = ABS(degrees - 180) / (gdouble) 180;
	if (degrees >= 90)
	{
		degrees += 180;
		if (priv->new && !priv->done)
		{
			gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->actor, priv->new);
			priv->old = priv->new;
			priv->new = NULL;
			priv->done = TRUE;
		}
	}
	
	clutter_actor_set_rotation(priv->actor, CLUTTER_Y_AXIS, degrees, 0, 0, 0);
	clutter_actor_set_scale(priv->actor, zoom, zoom);
	
	if (degrees >= 270 && priv->new)
	{
		timeline_stop(self);
		timeline_start(self);
	}
/*
self->z_rotation += 1;
if(self->z_rotation >= 360)
self->z_rotation = 0;

if (self->z_rotation >= 90)
{
self->i++;
if (self->covers[self->i] == NULL)
self->i = 0;
g_printf("Set cover to: %s\n", self->covers[self->i]);
clutter_texture_set_from_file(CLUTTER_TEXTURE(self->cover), self->covers[self->i], NULL);
self->z_rotation = -90;
}
clutter_actor_set_rotation(self->cover, CLUTTER_Y_AXIS, 
self->z_rotation, 0, clutter_actor_get_width(self->cover) / 2, 0);
*/
}
