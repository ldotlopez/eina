/*
 * plugins/clutty/eina-cover-clutter.c
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


#include "eina-cover-clutter.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaCoverClutter, eina_cover_clutter, GTK_CLUTTER_TYPE_EMBED)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_COVER_CLUTTER, EinaCoverClutterPrivate))

#define EFFECT_DURATION 1000
#define ZOOM_IN_MODE  CLUTTER_LINEAR
#define ZOOM_OUT_MODE CLUTTER_LINEAR
#define SPIN_MODE     CLUTTER_LINEAR

typedef struct _EinaCoverClutterPrivate EinaCoverClutterPrivate;

enum { CURR, NEXT, N_PBS };
enum { SPIN, ZOOM_IN, ZOOM_OUT, N_EFF };

struct _EinaCoverClutterPrivate {
	ClutterActor *texture;

	ClutterScore *score;

	ClutterAnimation *anims[N_EFF];
	GdkPixbuf *pbs[N_PBS];
};

enum {
	PROPERTY_COVER = 1,
	PROPERTY_ASIS
};

static gboolean
configure_event_cb(GtkWidget *self, GdkEventConfigure *ev, gpointer data);
static void
hierarchy_changed_cb(GtkWidget *self, GtkWidget *prev_tl, gpointer data);

static void
spin_1_completed_cb(ClutterTimeline *tl, EinaCoverClutter *self);

static void
eina_cover_clutter_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_ASIS:
		g_value_set_boolean(value, TRUE);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROPERTY_COVER:
		eina_cover_clutter_set_from_pixbuf(EINA_COVER_CLUTTER(object), GDK_PIXBUF(g_value_get_object(value)));
		break;
	case PROPERTY_ASIS:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_cover_clutter_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_cover_clutter_parent_class)->dispose (object);
}

static void
eina_cover_clutter_class_init (EinaCoverClutterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaCoverClutterPrivate));

	object_class->get_property = eina_cover_clutter_get_property;
	object_class->set_property = eina_cover_clutter_set_property;
	object_class->dispose = eina_cover_clutter_dispose;

    g_object_class_install_property(object_class, PROPERTY_COVER,
		g_param_spec_object("cover", "Cover", "Cover pixbuf",
		GDK_TYPE_PIXBUF, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
	g_object_class_install_property(object_class, PROPERTY_ASIS,
		g_param_spec_boolean("asis", "Asis", "As-is hint, ignored, allways TRUE",
		TRUE, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_cover_clutter_init (EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	priv->texture = clutter_texture_new();
	clutter_container_add_actor((ClutterContainer *) gtk_clutter_embed_get_stage((GtkClutterEmbed *) self), priv->texture);

	priv->score = clutter_score_new();

	ClutterTimeline *zoom_out = clutter_timeline_new(EFFECT_DURATION / 2);
	ClutterTimeline *zoom_in  = clutter_timeline_new(EFFECT_DURATION / 2);
	ClutterTimeline *spin_1   = clutter_timeline_new(EFFECT_DURATION / 2);
	ClutterTimeline *spin_2   = clutter_timeline_new(EFFECT_DURATION / 2);

	ClutterAlpha *a_z_out = clutter_alpha_new_full(zoom_out, ZOOM_OUT_MODE); 
	ClutterAlpha *a_z_in  = clutter_alpha_new_full(zoom_in,  ZOOM_IN_MODE); 
	ClutterAlpha *a_s_1   = clutter_alpha_new_full(spin_1,   SPIN_MODE); 
	ClutterAlpha *a_s_2   = clutter_alpha_new_full(spin_2,   SPIN_MODE); 

	ClutterBehaviour *b_z_out = clutter_behaviour_scale_new (a_z_out, 1.0, 1.0, 0.5, 0.5);
	ClutterBehaviour *b_z_in  = clutter_behaviour_scale_new (a_z_in,  0.5, 0.5, 1.0, 1.0);
	ClutterBehaviour *b_s_1   = clutter_behaviour_rotate_new(a_s_1,   CLUTTER_Y_AXIS, CLUTTER_ROTATE_CW, 0.0,   90.0);
	ClutterBehaviour *b_s_2   = clutter_behaviour_rotate_new(a_s_2,   CLUTTER_Y_AXIS, CLUTTER_ROTATE_CW, 270.0, 360.0);
	
	clutter_behaviour_apply(b_z_out, priv->texture);
	clutter_behaviour_apply(b_z_in, priv->texture);
	clutter_behaviour_apply(b_s_1, priv->texture);
	clutter_behaviour_apply(b_s_2, priv->texture);

	clutter_score_append(priv->score, NULL, spin_1);
	clutter_score_append(priv->score, spin_1, spin_2);
	clutter_score_append(priv->score, NULL, zoom_out);
	clutter_score_append(priv->score, zoom_out, zoom_in);

	g_signal_connect(spin_1, "completed", (GCallback) spin_1_completed_cb, self);
	
	g_signal_connect(self, "configure-event",   (GCallback) configure_event_cb, NULL);
	g_signal_connect(self, "hierarchy-changed", (GCallback) hierarchy_changed_cb, NULL);
}

EinaCoverClutter*
eina_cover_clutter_new (void)
{
	return g_object_new (EINA_TYPE_COVER_CLUTTER, NULL);
}

static void
start_animation(EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	if (!gtk_widget_get_visible((GtkWidget *) self) || clutter_score_is_playing(priv->score))
		return;
	clutter_score_start(priv->score);
}

void
eina_cover_clutter_set_from_pixbuf(EinaCoverClutter *self, GdkPixbuf *pixbuf)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(pixbuf != NULL);

	// Put on next?
	if (priv->pbs[CURR])
	{
		if (priv->pbs[NEXT])
			g_object_unref(priv->pbs[NEXT]);
		priv->pbs[NEXT] = pixbuf;
		start_animation(self);
		return;
	}
	else
	{
		priv->pbs[CURR] = pixbuf;
		gtk_widget_queue_resize((GtkWidget *) self);
	}
}

static gboolean
configure_event_cb(GtkWidget *widget, GdkEventConfigure *ev, gpointer data)
{
	EinaCoverClutter *self = EINA_COVER_CLUTTER(widget);
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);

	if (priv->pbs[CURR] == NULL)
		return FALSE;

	if ((ev->width <= 1) || (ev->height <= 1))
		return FALSE;

	// gint pbw = gdk_pixbuf_get_width (priv->pbs[CURR]);
	// gint pbh = gdk_pixbuf_get_height(priv->pbs[CURR]);
	// clutter_actor_set_size(priv->texture, pbw, pbh);
	/*
	gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->texture, priv->pbs[CURR], NULL);
	clutter_actor_set_size(priv->texture, ev->width, ev->height);
	*/
	clutter_actor_set_size(priv->texture, ev->width, ev->height);
	gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->texture, priv->pbs[CURR], NULL);

	clutter_actor_set_anchor_point_from_gravity(priv->texture, CLUTTER_GRAVITY_CENTER);
	clutter_actor_set_position(priv->texture, ev->width / (gfloat) 2, ev->width / (gfloat) 2);

	return FALSE;
}

static void
hierarchy_changed_cb(GtkWidget *self, GtkWidget *prev_tl, gpointer data)
{
	if (prev_tl != NULL)
		return;

	GdkColor gc = gtk_widget_get_style(gtk_widget_get_parent(self))->bg[GTK_STATE_NORMAL];
	ClutterStage *stage = (ClutterStage *) gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(self));
	ClutterColor cc = { gc.red / 255, gc.green / 255, gc.blue / 255, 255 };
	clutter_stage_set_color(stage, &cc);
}

static void
spin_1_completed_cb(ClutterTimeline *tl, EinaCoverClutter *self)
{
	EinaCoverClutterPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv->pbs[NEXT] != NULL);

	g_object_unref(priv->pbs[CURR]);
	priv->pbs[CURR] = priv->pbs[NEXT];
	priv->pbs[NEXT] = NULL;
	gtk_clutter_texture_set_from_pixbuf((ClutterTexture *) priv->texture, priv->pbs[CURR], NULL);
}

