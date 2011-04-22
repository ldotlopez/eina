/*
 * plugins/fieshta/fieshta-stage.c
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

#include "fieshta-stage.h"
#include <math.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (FieshtaStage, fieshta_stage, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), FIESTA_STAGE_TYPE_STAGE, FieshtaStagePrivate))

typedef struct _FieshtaStagePrivate FieshtaStagePrivate;

struct _FieshtaStagePrivate {
	gint slots;
	gint w, h;
	GList  *actors;
	gfloat *scales;
	gint   *sizes;
	gint   *positions;
};

void
set_apparent_size(ClutterActor *actor, gint w, gint h);
#define fieshta_stage_get_scale_for_slot(self,actor,slot) GET_PRIVATE(self)->scales[slot] * GET_PRIVATE(self)->h / clutter_actor_get_height(actor)

static void
fieshta_stage_dispose (GObject *object)
{
	G_OBJECT_CLASS (fieshta_stage_parent_class)->dispose (object);
}

static void
fieshta_stage_class_init (FieshtaStageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (FieshtaStagePrivate));

	object_class->dispose = fieshta_stage_dispose;
}

static void
fieshta_stage_init (FieshtaStage *self)
{
}

FieshtaStage*
fieshta_stage_new (void)
{
	return g_object_new (FIESTA_STAGE_TYPE_STAGE, NULL);
}

ClutterActor *
fieshta_stage_get_nth(FieshtaStage* self, guint slot)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	GList *l = g_list_last(priv->actors);
	gint i = priv->slots - 1;
	while (l && (i != slot))
	{
		l = l->prev;
		i--;
	}
	g_return_val_if_fail(l != NULL, NULL);
	return (ClutterActor *) l->data;
}

void
fieshta_stage_set_slots(FieshtaStage *self, guint n)
{
	g_return_if_fail(n > 0);
	g_return_if_fail((n % 2) == 1);
	FieshtaStagePrivate *priv = GET_PRIVATE(self);

	if (priv->scales)    g_free(priv->scales);
	if (priv->sizes)     g_free(priv->sizes);
	if (priv->positions) g_free(priv->positions);

	priv->slots     = n;
	priv->w         = clutter_actor_get_width((ClutterActor *) self);
	priv->h         = clutter_actor_get_height((ClutterActor *) self);
	priv->scales    = g_new0(gfloat, n);
	priv->sizes     = g_new0(gint, n);
	priv->positions = g_new0(gint, n);

	gfloat base_scale = 0.25;
	gint i;
	for (i = 0; i < (n / 2) + 1; i++)
	{
		gint sa = (n/2) + i;
		gint sb = (n/2) - i;

		priv->scales[sa] = base_scale / pow(2,i);
		priv->scales[sb] = priv->scales[sa];

		priv->sizes[sa] = (gint)(priv->scales[sa] * priv->h);
		priv->sizes[sb] = priv->sizes[sa];
	}

	gint margin = 10;
	gint x;
	for (x = (n/2) + 1; x < n; x++)
	{
		priv->positions[x] = priv->positions[x-1] + (priv->sizes[x-1] / 2) + (priv->sizes[x] / 2) + margin;
		priv->positions[n-x-1] = -priv->positions[x];
	}
	for (i = 0; i < priv->slots; i++)
		priv->positions[i] = (priv->h / 2)+ priv->positions[i];
}

static void
fieshta_stage_translate(FieshtaStage *self, ClutterActor *actor, gint i)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(g_list_find(priv->actors, actor));

	gdouble scale = fieshta_stage_get_scale_for_slot(self, actor, i);
	clutter_actor_animate(actor, CLUTTER_LINEAR, 500,
		"x", 1280/2,
		"y", priv->positions[i],
		"scale-x", scale,
		"scale-y", scale,
		NULL);
/*
	ClutterTimeline *timeline = clutter_timeline_new(500);
	
	ClutterAlpha *alpha = clutter_alpha_new();
	clutter_alpha_set_timeline(alpha, timeline);
	clutter_alpha_set_func(alpha, CLUTTER_LINEAR, NULL, NULL);

	ClutterKnot knots[2] = {
		{ 1280/2, clutter_actor_get_y(actor) },
	    { 1280/2, priv->positions[i]}
	};
	ClutterBehaviour *p = clutter_behaviour_path_new(alpha, knots, 2);
	clutter_behaviour_apply(p, actor);

	gdouble curr_sx, curr_sy;
	clutter_actor_get_scale(actor, &curr_sx, &curr_sy);

	gdouble scale = fieshta_stage_get_scale_for_slot(self, actor, i);
	ClutterBehaviour *s = clutter_behaviour_scale_new(alpha, 
		curr_sx, curr_sy,
		scale, scale
	);
	clutter_behaviour_apply(s, actor);

	clutter_timeline_start(timeline);
	*/
}

#if 0
static void
timeline_completed_cb(ClutterTimeline *t, FieshtaStage *self)
{
	ClutterActor *actor = (ClutterActor *) g_object_get_data((GObject *) t, "actor");
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	if (!g_list_find(priv->actors, actor))
		clutter_container_remove_actor(
			(ClutterContainer *) clutter_actor_get_parent(actor),
			actor);
	
	g_object_unref((GObject *) g_object_get_data((GObject *) t, "behaviour"));
	g_object_unref((GObject *) t);
}
#endif

static void
fieshta_stage_add(FieshtaStage *self, ClutterActor *actor, guint pos)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);

	// Set position
	gint x, y;
	x = 1280 / 2;
	y = priv->positions[pos];
	clutter_actor_set_position(actor, x, y);

	// Calculate scale
	gdouble sy;
	sy = priv->scales[pos] * priv->h / clutter_actor_get_height(actor);

	// Create animation
	clutter_actor_animate(actor, CLUTTER_LINEAR, 500,
		"scale-x", sy, 
		"scale-y", sy,
		NULL);
	/*
	ClutterTimeline *timeline = clutter_timeline_new_for_duration(500);
	g_signal_connect(timeline, "completed", (GCallback) timeline_completed_cb, self);
	
	ClutterAlpha *alpha = clutter_alpha_new();
	clutter_alpha_set_timeline(alpha, timeline);
	clutter_alpha_set_func(alpha, CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

	ClutterBehaviour *s = clutter_behaviour_scale_new(alpha,
		0.0, 0.0,
		sy, sy
		);

	clutter_actor_set_scale(actor, 0.0, 0.0);
	clutter_container_add_actor((ClutterContainer *) self, actor);
	clutter_actor_show(actor);

	clutter_behaviour_apply(s, actor);
	g_object_set_data((GObject *) timeline, "behaviour", s);
	g_object_set_data((GObject *) timeline, "actor",     actor);
	clutter_timeline_start(timeline);
	*/
}

static void
fieshta_stage_remove(FieshtaStage *self, ClutterActor *actor)
{
	// Create animation
	clutter_actor_animate(actor, CLUTTER_LINEAR, 500,
		"scale-x", 0.0,
		"scale-y", 0.0,
		NULL);
	/*
	ClutterTimeline *timeline = clutter_timeline_new_for_duration(500);
	
	ClutterAlpha *alpha = clutter_alpha_new();
	clutter_alpha_set_timeline(alpha, timeline);
	clutter_alpha_set_func(alpha, CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

	gdouble sx, sy;
	clutter_actor_get_scale(actor, &sx, &sy);
	ClutterBehaviour *s = clutter_behaviour_scale_new(alpha,
		sx, sy,
		0.0, 0.0
		);
	g_signal_connect(timeline, "completed", (GCallback) timeline_completed_cb, self);

	clutter_behaviour_apply(s, actor);
	g_object_set_data((GObject *) timeline, "behaviour", s);
	g_object_set_data((GObject *) timeline, "actor",     actor);
	clutter_timeline_start(timeline);
	*/
}

void
fieshta_stage_push(FieshtaStage *self, ClutterActor *actor_)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);

	// Delete extra actors from head
	while (g_list_length(priv->actors) >= priv->slots)
	{
		fieshta_stage_remove(self, (ClutterActor *) priv->actors->data);
		priv->actors = g_list_delete_link(priv->actors, priv->actors);
	}

	// Move actors from i to i-1
	gint i = priv->slots - 1;
	GList *tmp = g_list_last(priv->actors);
	while (tmp && (i > 0))
	{
		fieshta_stage_translate(self, tmp->data, --i);
		tmp = tmp->prev;
	}

	// Add new actor
	fieshta_stage_add(self, actor_, priv->slots - 1);
	priv->actors = g_list_append(priv->actors, actor_);
}

