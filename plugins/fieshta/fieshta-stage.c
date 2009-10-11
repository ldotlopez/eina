#include "fieshta-stage.h"
#include <math.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (FieshtaStage, fieshta_stage, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), FIESTA_STAGE_TYPE_STAGE, FieshtaStagePrivate))

typedef struct _FieshtaStagePrivate FieshtaStagePrivate;

struct _FieshtaStagePrivate {
	gint slots;
	ClutterActor **actors;
	gfloat       *scales;
	gint         *sizes;
	gint         *positions;
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
		clutter_container_remove_actor((ClutterContainer *) self, priv->actors[index]);
 
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
