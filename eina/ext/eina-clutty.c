/*
 * eina/ext/eina-clutty.c
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

#include <eina/ext/eina-clutty.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaClutty, eina_clutty, GTK_CLUTTER_TYPE_EMBED)

#define CLUTTY_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_CLUTTY, EinaCluttyPrivate))

typedef struct _EinaCluttyPrivate EinaCluttyPrivate;

struct _EinaCluttyPrivate {
	ClutterActor *actor;
	ClutterTimeline *timeline;
	GdkPixbuf *old, *new;
	guint duration;
	gboolean done;
};

enum {
	EINA_CLUTTY_PROPERTY_DURATION = 1,
	EINA_CLUTTY_PROPERTY_PIXBUF
};

void timeline_start(EinaClutty *self);
void timeline_stop(EinaClutty *self);
void timeline_new_frame_cb (ClutterTimeline *timeline, gint frame_num, EinaClutty *self);

static void
eina_clutty_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaClutty *self = (EinaClutty *) object;
	switch (property_id)
	{
	case EINA_CLUTTY_PROPERTY_DURATION:
		g_value_set_uint(value, eina_clutty_get_duration(self));
		break;

	case EINA_CLUTTY_PROPERTY_PIXBUF:
		g_value_set_object(value, (gpointer) eina_clutty_get_pixbuf(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_clutty_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaClutty *self = (EinaClutty *) object;
	switch (property_id)
	{
	case EINA_CLUTTY_PROPERTY_DURATION:
		eina_clutty_set_duration(self, g_value_get_uint(value));
		break;

	case EINA_CLUTTY_PROPERTY_PIXBUF:
		eina_clutty_set_pixbuf(self, GDK_PIXBUF(g_value_get_object(value)));
		break;

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

	g_object_class_install_property(object_class, EINA_CLUTTY_PROPERTY_DURATION,
		g_param_spec_uint("duration", "Duration", "Duration in msecs",
		1, G_MAXUINT, 2000,
		G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, EINA_CLUTTY_PROPERTY_PIXBUF,
		g_param_spec_object("pixbuf", "Pixbuf", "Pixbuf to display",
		GDK_TYPE_PIXBUF,
		G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_clutty_init (EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);

	priv->done     = FALSE;
	priv->old      = NULL;
	priv->new      = NULL;
	priv->actor    = clutter_texture_new();
	priv->timeline = clutter_timeline_new(priv->duration);

	clutter_container_add_actor((ClutterContainer*) gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self)), priv->actor);
	clutter_timeline_set_loop(priv->timeline, FALSE); 

	g_signal_connect (priv->timeline, "new-frame", G_CALLBACK(timeline_new_frame_cb), self);
}

EinaClutty*
eina_clutty_new (void)
{	
	return g_object_new (EINA_TYPE_CLUTTY, NULL);
}

void eina_clutty_set_duration(EinaClutty *self, guint duration)
{
	g_return_if_fail(duration >= 0);

	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	priv->duration = duration;
	clutter_timeline_set_duration(priv->timeline, duration);
}

guint eina_clutty_get_duration(EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	return priv->duration;
}

void
eina_clutty_set_pixbuf(EinaClutty *self, GdkPixbuf *pixbuf)
{
	EinaCluttyPrivate *priv  = CLUTTY_PRIVATE(self);
	ClutterActor      *stage = gtk_clutter_embed_get_stage((GtkClutterEmbed *) self);

	// Set directly on stage
	if (!priv->old)
	{
		priv->old = pixbuf;
		gtk_clutter_texture_set_from_pixbuf((ClutterTexture*) priv->actor, priv->old, NULL);
		clutter_actor_set_anchor_point(priv->actor,
			clutter_actor_get_width(priv->actor) / 2,
			clutter_actor_get_height(priv->actor) / 2);
		clutter_actor_set_position(priv->actor,
			clutter_actor_get_width(stage) / 2,
			clutter_actor_get_height(stage) / 2);
		return;
	}

	// Drop old new if any, hold new, start pipeline
	// if (priv->new)
	// 	g_object_unref(priv->new);
	priv->new = pixbuf;

	timeline_start(self);
}

GdkPixbuf*
eina_clutty_get_pixbuf(EinaClutty *self)
{
	EinaCluttyPrivate *priv = CLUTTY_PRIVATE(self);
	return (priv->new ? priv->new : priv->old);
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
	gdouble degrees = ((gdouble) 180) / 
		// Total frames
		(gint) ((clutter_timeline_get_duration(timeline)) / (gdouble) 1000) *
		frame_num;

	if (degrees >= 90)
	{
		degrees += 180;
		if (priv->new && !priv->done)
		{
			gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->actor, priv->new, NULL);
			priv->old = priv->new;
			priv->new = NULL;
			priv->done = TRUE;
		}
	}

	gdouble zoom = CLAMP((ABS(180 - degrees) - 90) / 90, 0.5, 1);
	ClutterActor *stage = (ClutterActor *) gtk_clutter_embed_get_stage((GtkClutterEmbed *) self);
	gint w = clutter_actor_get_width(stage);
	gint h = clutter_actor_get_height(stage);
	
	clutter_actor_set_size(priv->actor, w * zoom, h * zoom);
	clutter_actor_set_anchor_point(priv->actor, w * zoom / 2, h * zoom / 2);
	
	clutter_actor_set_rotation(priv->actor, CLUTTER_Y_AXIS, degrees, 0, 0, 0);
	
	if (degrees >= 270 && priv->new)
	{
		timeline_stop(self);
		timeline_start(self);
	}
}

