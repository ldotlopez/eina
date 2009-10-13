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
	// ClutterActor **actors;
	GList  *actors;
	gfloat *scales;
	gint   *sizes;
	gint   *positions;
};
void
set_apparent_size(ClutterActor *actor, gint w, gint h);

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
		g_printf("Scale for %d and %d: %f\n", sa, sb, priv->scales[sa]);
		g_printf("Sizes for %d and %d: %d\n", sa, sb, priv->sizes[sa]);
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
fieshta_stage_position(FieshtaStage *self, ClutterActor *actor, gint i)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);

	gint x, y;
	x = priv->w / 2;
	y = priv->positions[i];

	guint ow, oh;
	clutter_actor_get_size(actor, &ow, &oh);

	gfloat nh, nw;
	nh = priv->h * priv->scales[i];
	nw = (gint) (ow * ((float) nh/oh));

	if (g_list_find(priv->actors, actor))
	{
		ClutterTimeline *timeline = clutter_timeline_new_for_duration(1000);
	
		ClutterAlpha *alpha = clutter_alpha_new();
		clutter_alpha_set_timeline(alpha, timeline);
		clutter_alpha_set_func(alpha, CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

		ClutterKnot knots[2] = {
			{ 1280/2, priv->positions[i-1]},
		    { 1280/2, priv->positions[i]}
		};
		ClutterBehaviour *p = clutter_behaviour_path_new(alpha, knots, 2);
		clutter_behaviour_apply(p, actor);


		gdouble curr_sx, curr_sy;
		clutter_actor_get_scale(actor, &curr_sx, &curr_sy);
		ClutterBehaviour *s = clutter_behaviour_scale_new(alpha, 
			curr_sx, curr_sy,
			((gdouble)nw)/ow, ((gdouble)nh)/oh
		);
		clutter_behaviour_apply(s, actor);

		clutter_timeline_start(timeline);
		g_printf("Animation required\n");
	}
	else
	{
		clutter_actor_set_position(actor, x, y);
		clutter_actor_set_scale(actor, (float) nw / ow, (float) nh /oh);
		g_printf("Actor %p is at (%d, %d) size %fx%f\n", actor, x, y, nh, nw);
	}
}

void
fieshta_stage_push(FieshtaStage *self, ClutterActor *actor_)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	while (g_list_length(priv->actors) > priv->slots)
	{
		g_printf("Deleting actor\n");
		GList *l = g_list_last(priv->actors);
		clutter_container_remove_actor((ClutterContainer *) self, (ClutterActor *) l->data);
		priv->actors = g_list_delete_link(priv->actors, l);
	}
 
 	gint i;
	for (i = 0; i < g_list_length(priv->actors); i++)
	{
		g_printf("Move %d to %d\n", i, i + 1);
		fieshta_stage_position(self, g_list_nth_data(priv->actors, i), i+1);
	}

	clutter_container_add_actor((ClutterContainer *) self, actor_);
	fieshta_stage_position(self, actor_, 0);
	priv->actors = g_list_prepend(priv->actors, actor_);
	clutter_actor_show(actor_);
}


#if 0
void
fieshta_stage_set_slots(FieshtaStage *self, guint slots)
{
	g_return_if_fail(slots > 0);
	g_return_if_fail((slots % 2) == 1);

	gint i;
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	priv->slots = slots;
	
	priv->actors = g_new0(ClutterActor*, slots);

	if (priv->scales)
		g_free(priv->scales);
	if (priv->sizes)
		g_free(priv->sizes);
	if (priv->positions)
		g_free(priv->positions);

	priv->scales    = g_new0(gfloat, slots);
	priv->sizes     = g_new0(gint,   slots);
	priv->positions = g_new0(gint,   slots);

	for (i = 0; i <= (slots / 2); i++)
	{
		priv->scales[i] = (gfloat) 1 / pow(2, i+2);
		priv->sizes[i]  = (gint) (priv->scales[i] * clutter_actor_get_height((ClutterActor *) self));
		g_printf("# Scale for %d: %f\n", i,  priv->scales[i]);
		g_printf("# Size for %d: %d\n", i, priv->sizes[i]);
	}

	gint margin = 10;
	priv->positions[0] = 0;
	for (i = 1; i <= slots / 2; i++)
	{
		priv->positions[i] = priv->positions[i-1] + priv->sizes[i-1] / 2 + priv->sizes[i] / 2  + margin;
		g_printf("# Position for %d: %d\n", i, priv->positions[i]);
	}
}
ClutterActor *
fieshta_stage_get_nth(FieshtaStage* self, guint slot)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);

	g_return_val_if_fail(slot >= 0, NULL);
	g_return_val_if_fail(slot < priv->slots, NULL);
	return priv->actors[slot];
}

void
fieshta_stage_set_nth(FieshtaStage* self, guint index, ClutterActor* actor)
{
	FieshtaStagePrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(index >= 0);
	g_return_if_fail(index < priv->slots);

	if (priv->actors[index])
	{
		clutter_container_remove_actor((ClutterContainer *) self, priv->actors[index]);
		priv->actors[index] = NULL;
 	}

 	if (!actor)
		return;

 	gint slot = ((int) (priv->slots / 2)) - index;
	priv->actors[index] = actor;

	// Set apparent size
	gfloat scale = priv->sizes[ABS(slot)] / (gfloat) clutter_actor_get_height(actor);
	set_apparent_size(actor, clutter_actor_get_width(actor) * scale, priv->sizes[ABS(slot)]);

	// Put in corrent position
	gint pad = priv->positions[ABS(slot)];
	if (slot < 0)
		pad = -pad;
	clutter_actor_set_position(actor,
		clutter_actor_get_width((ClutterActor *) self) / 2,
		clutter_actor_get_height((ClutterActor *) self) / 2 + pad);
	clutter_container_add_actor((ClutterContainer *) self, actor);
	clutter_actor_show(actor);
}

void
set_apparent_size(ClutterActor *actor, gint w, gint h)
{
	guint w_, h_;
	
	clutter_actor_get_size(actor, &w_, &h_);
	clutter_actor_set_scale(actor, w / (gfloat) w_, h / (gfloat) h_);
}
#endif
