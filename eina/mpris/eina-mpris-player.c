#include "eina-mpris-player.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaMprisPlayer, eina_mpris_player, G_TYPE_OBJECT)

struct _EinaMprisPlayerPrivate {
	LomoPlayer *lomo;
};

enum
{
	PROP_LOMO_PLAYER = 1,
	PROP_CAN_QUIT,
	PROP_CAN_RAISE,
	PROP_HAS_TRACK_LIST,
	PROP_IDENTITY,
	PROP_DESKTOP_ENTRY,
	PROP_SUPPORTED_URI_SCHEMES,
	PROP_SUPPORTED_MIME_TYPES
};

static void
set_lomo_player(EinaMprisPlayer *self, LomoPlayer *lomo);

static void
eina_mpris_player_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		g_value_set_object(value, eina_mpris_player_get_lomo_player(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_CAN_QUIT:
		g_value_set_boolean(value, eina_mpris_player_get_can_quit(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_CAN_RAISE:
		g_value_set_boolean(value, eina_mpris_player_get_can_raise(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_HAS_TRACK_LIST:
		g_value_set_boolean(value, eina_mpris_player_get_has_track_list(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_IDENTITY:
		g_value_set_static_string(value, eina_mpris_player_get_identify(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_DESKTOP_ENTRY:
		g_value_set_static_string(value, eina_mpris_player_get_desktop_entry(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_SUPPORTED_URI_SCHEMES:
		g_value_set_boxed(value, eina_mpris_player_get_supported_uri_schemes(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_SUPPORTED_MIME_TYPES:
		g_value_set_boxed(value, eina_mpris_player_get_supported_mime_types(EINA_MPRIS_PLAYER(object)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_mpris_player_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		set_lomo_player(EINA_MPRIS_PLAYER(object), g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_mpris_player_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_mpris_player_parent_class)->dispose (object);
}

static void
eina_mpris_player_class_init (EinaMprisPlayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaMprisPlayerPrivate));

	object_class->get_property = eina_mpris_player_get_property;
	object_class->set_property = eina_mpris_player_set_property;
	object_class->dispose = eina_mpris_player_dispose;

	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object( "lomo-player", "lomo-player", "lomo-player",
			LOMO_TYPE_PLAYER, G_PARAM_READABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

static void
eina_mpris_player_init (EinaMprisPlayer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerPrivate);
}

EinaMprisPlayer*
eina_mpris_player_new (LomoPlayer *lomo)
{
	return g_object_new (EINA_TYPE_MPRIS_PLAYER, "lomo-player", lomo, NULL);
}

gboolean
eina_mpris_player_get_can_quit(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return FALSE;
}

gboolean
eina_mpris_player_get_can_raise(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return FALSE;
}

gboolean
eina_mpris_player_get_has_track_list(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return FALSE;
}

const gchar*
eina_mpris_player_get_identify(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return "Eina music player";
}

const gchar*
eina_mpris_player_get_desktop_entry(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return NULL;
}
const gchar* const *
eina_mpris_player_get_supported_uri_schemes(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return NULL;
}

const gchar* const *
eina_mpris_player_get_supported_mime_types(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
	return NULL;
}

LomoPlayer *
eina_mpris_player_get_lomo_player(EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), NULL);
	return LOMO_PLAYER(self->priv->lomo);
}

static void
set_lomo_player(EinaMprisPlayer *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	gel_free_and_invalidate(self->priv->lomo, NULL, g_object_unref);
	self->priv->lomo = g_object_ref(lomo);
}

void
eina_mpris_player_raise(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
}

void
eina_mpris_player_quit(EinaMprisPlayer *self)
{
	g_warning("%s", __FUNCTION__);
}
