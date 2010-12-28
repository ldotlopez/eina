#include "eina-fiestha-behaviour.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaFieshtaBehaviour, eina_fieshta_behaviour, G_TYPE_OBJECT)

struct _EinaFieshtaBehaviourPrivate {
	LomoPlayer *lomo;
	EinaFieshtaBehaviourOptions options;
};

enum {
	PROP_LOMO_PLAYER = 1,
	PROP_OPTIONS
};

static void
set_lomo_player(EinaFieshtaBehaviour *self, LomoPlayer *lomo);

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent event, gpointer ret, EinaFieshtaBehaviour *self);


static void
eina_fieshta_behaviour_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		g_value_set_object(value, eina_fieshta_behaviour_get_lomo_player(EINA_FIESHTA_BEHAVIOUR(object)));
		return;
	case PROP_OPTIONS:
		g_value_set_int(value, eina_fieshta_behaviour_get_options(EINA_FIESHTA_BEHAVIOUR (object)));
		return;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_fieshta_behaviour_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		set_lomo_player(EINA_FIESHTA_BEHAVIOUR(object), g_value_get_object(value));
		return;
	case PROP_OPTIONS:
		eina_fieshta_behaviour_set_options(EINA_FIESHTA_BEHAVIOUR(object), (EinaFieshtaBehaviourOptions) g_value_get_int(value));
		return;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_fieshta_behaviour_dispose (GObject *object)
{
	EinaFieshtaBehaviour *self = EINA_FIESHTA_BEHAVIOUR(object);
	
	if (self->priv->lomo)
	{
		g_object_unref(self->priv->lomo);
		lomo_player_hook_remove(self->priv->lomo, (LomoPlayerHook) lomo_hook_cb);
	}

	G_OBJECT_CLASS (eina_fieshta_behaviour_parent_class)->dispose (object);
}

static void
eina_fieshta_behaviour_class_init (EinaFieshtaBehaviourClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaFieshtaBehaviourPrivate));

	object_class->get_property = eina_fieshta_behaviour_get_property;
	object_class->set_property = eina_fieshta_behaviour_set_property;
	object_class->dispose = eina_fieshta_behaviour_dispose;

	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player",
			LOMO_TYPE_PLAYER,
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
	
	g_object_class_install_property(object_class, PROP_OPTIONS,
		g_param_spec_int( "options", "options", "options",
			-1, 0x1111, EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT,
			G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

static void
eina_fieshta_behaviour_init (EinaFieshtaBehaviour *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviourPrivate));
}

EinaFieshtaBehaviour*
eina_fieshta_behaviour_new (LomoPlayer *lomo, EinaFieshtaBehaviourOptions options)
{
	return g_object_new (EINA_TYPE_FIESHTA_BEHAVIOUR,
		"lomo-player", lomo,
		"options", options,
		NULL);
}

static void
set_lomo_player(EinaFieshtaBehaviour *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_FIESHTA_BEHAVIOUR(self));

	EinaFieshtaBehaviourPrivate *priv = self->priv;
	g_return_if_fail(priv->lomo == NULL);

	priv->lomo = g_object_ref(lomo);
	lomo_player_hook_add(lomo, (LomoPlayerHook) lomo_hook_cb, self);
}

/**
 * eina_fieshta_behaviour_get_lomo_player:
 * @self: A #EinaFieshtaBehaviour
 *
 * Get the current #LomoPlayer object from the behaviour
 *
 * Returns: (transfer none): The #LomoPlayer
 **/
LomoPlayer *
eina_fieshta_behaviour_get_lomo_player(EinaFieshtaBehaviour *self)
{
	g_return_val_if_fail(EINA_IS_FIESHTA_BEHAVIOUR(self), NULL);
	return self->priv->lomo;
}

EinaFieshtaBehaviourOptions
eina_fieshta_behaviour_get_options(EinaFieshtaBehaviour *self)
{
	g_return_val_if_fail(EINA_IS_FIESHTA_BEHAVIOUR(self), -1);
	return self->priv->options;
}

void
eina_fieshta_behaviour_set_options(EinaFieshtaBehaviour *self, EinaFieshtaBehaviourOptions options)
{
	g_return_if_fail(EINA_IS_FIESHTA_BEHAVIOUR(self));
	self->priv->options = options;
}

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaFieshtaBehaviour *self)
{
	EinaFieshtaBehaviourPrivate *priv = self->priv;

	// Block seek-ff
	if ((ev.type == LOMO_PLAYER_HOOK_SEEK)
		&& (priv->options & EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF)
		&& (ev.old < ev.new))
	{
		// g_debug("seek-ff blocked");
		return TRUE;
	}

	// Block seek-rew
	if ((ev.type == LOMO_PLAYER_HOOK_SEEK)
		&& (priv->options & EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW)
		&& (ev.old > ev.new))
	{
		// g_debug("seek-rew blocked");
		return TRUE;
	}

	// Block insert but at the end
	if ((ev.type == LOMO_PLAYER_HOOK_INSERT)
		&& (priv->options & EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT)
		&& (ev.pos != lomo_player_get_total(lomo)))
	{
		// g_debug("insert blocked");
		return TRUE;
	}

	// Block change
	if ((ev.type == LOMO_PLAYER_HOOK_CHANGE)
		&& (priv->options & EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE))
	{
		if (ev.to == (ev.from + 1))
			return FALSE;
		if ((ev.from == lomo_player_get_total(lomo) - 1) && (ev.to == 0))
			return FALSE;
		// g_debug("change from %d to %d blocked", ev.from, ev.to);
		return TRUE;
	}

	return FALSE;
}

