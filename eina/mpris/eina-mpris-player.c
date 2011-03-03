#include "eina-mpris-player.h"
#include <gel/gel.h>
#include <glib/gi18n.h>
#include "eina-mpris-player-spec.h"

G_DEFINE_TYPE (EinaMprisPlayer, eina_mpris_player, G_TYPE_OBJECT)

struct _EinaMprisPlayerPrivate {
	LomoPlayer      *lomo;
	gchar           *bus_name_suffix;
	GDBusConnection *conn;
	GDBusNodeInfo   *nodeinfo;
	guint bus_id, root_id, player_id;
};

enum
{
	PROP_LOMO_PLAYER = 1,
	PROP_BUS_NAME_SUFFIX
#if 0
	PROP_CAN_QUIT,
	PROP_CAN_RAISE,
	PROP_HAS_TRACK_LIST,
	PROP_IDENTITY,
	PROP_DESKTOP_ENTRY,
	PROP_SUPPORTED_URI_SCHEMES,
	PROP_SUPPORTED_MIME_TYPES
#endif
};

static void
set_lomo_player(EinaMprisPlayer *self, LomoPlayer *lomo);
static void
set_bus_name_suffix(EinaMprisPlayer *self, const gchar *bus_name_suffix);

static void
server_name_lost_cb (GDBusConnection *connection, const gchar *name, gpointer user_data);
static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data);
static void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self);
static GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self);
static gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self);
static void
player_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self);
static GVariant*
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self);
static gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self);

GDBusInterfaceVTable root_vtable =
{
	(GDBusInterfaceMethodCallFunc)  root_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) root_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) root_set_property_cb
};

GDBusInterfaceVTable player_vtable =
{
	(GDBusInterfaceMethodCallFunc)  player_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) player_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) player_set_property_cb
};

static void
eina_mpris_player_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		g_value_set_object(value, eina_mpris_player_get_lomo_player(EINA_MPRIS_PLAYER(object)));
		break;

	case PROP_BUS_NAME_SUFFIX:
		g_value_set_string(value, eina_mpris_player_get_bus_name_suffix(EINA_MPRIS_PLAYER(object)));
		break;
#if 0
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
#endif
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
	case PROP_BUS_NAME_SUFFIX:
		set_bus_name_suffix(EINA_MPRIS_PLAYER(object), g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_mpris_player_dispose (GObject *object)
{
	EinaMprisPlayer *self = EINA_MPRIS_PLAYER(object);

	// FIXME: Disconnect signals
	// FIXME: Merge this code with complete_setup_error goto

	gel_free_and_invalidate(self->priv->bus_id, 0, g_bus_unown_name);

	if (self->priv->player_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->player_id);
		self->priv->player_id = 0;
	}
	if (self->priv->root_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->root_id);
		self->priv->root_id = 0;
	}

	gel_free_and_invalidate(self->priv->nodeinfo, NULL, g_dbus_node_info_unref);
	gel_free_and_invalidate(self->priv->conn,     NULL, g_object_unref);

	gel_free_and_invalidate(self->priv->lomo, NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->bus_name_suffix, NULL, g_free);

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
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player",
			LOMO_TYPE_PLAYER, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(object_class, PROP_BUS_NAME_SUFFIX,
		g_param_spec_string("bus-name-suffix", "bus-name-suffix", "bus-name-suffix",
			NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

static void
eina_mpris_player_init (EinaMprisPlayer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerPrivate);
}

EinaMprisPlayer*
eina_mpris_player_new (LomoPlayer *lomo, const gchar *bus_name_suffix)
{
	return g_object_new (EINA_TYPE_MPRIS_PLAYER,
		"lomo-player", lomo,
		"bus-name-suffix", bus_name_suffix,
		NULL);
}

static void
complete_setup(EinaMprisPlayer *self)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));

	GError *error = NULL;

	self->priv->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!self->priv->conn)
	{
		g_warning(_("Unable to get session bus: '%s'"), error->message);
		goto complete_setup_error;
	}

	self->priv->nodeinfo = g_dbus_node_info_new_for_xml(mpris_spec_xml, &error);
	if (!self->priv->nodeinfo)
	{
		g_warning(_("Unable to parse MPRIS spec XML: '%s'"), error->message);
		goto complete_setup_error;
	}

	self->priv->root_id = g_dbus_connection_register_object(self->priv->conn,
		MPRIS_SPEC_OBJECT_PATH,
        g_dbus_node_info_lookup_interface(self->priv->nodeinfo, MPRIS_SPEC_ROOT_INTERFACE),
        &root_vtable,
        self,
        NULL,
        &error);
	if (!self->priv->root_id)
    {
        g_warning(_("Unable to register interface %s: '%s'"), MPRIS_SPEC_ROOT_INTERFACE, error->message);
		goto complete_setup_error;
    }

	self->priv->player_id = g_dbus_connection_register_object(self->priv->conn,
		MPRIS_SPEC_OBJECT_PATH,
		g_dbus_node_info_lookup_interface(self->priv->nodeinfo, MPRIS_SPEC_PLAYER_INTERFACE),
		&player_vtable,
		self,
		NULL,
		&error);
	if (!self->priv->player_id)
	{
		g_warning(_("Unable to register interface %s: '%s'"), MPRIS_SPEC_ROOT_INTERFACE, error->message);
		goto complete_setup_error;
	}

	gchar *bus_name = g_strconcat(MPRIS_SPEC_BUS_NAME_PREFIX, ".", self->priv->bus_name_suffix, NULL);
	self->priv->bus_id = g_bus_own_name(G_BUS_TYPE_SESSION,
		bus_name,
		G_BUS_NAME_OWNER_FLAGS_NONE,
		NULL,
		(GBusNameAcquiredCallback) server_name_acquired_cb,
		(GBusNameLostCallback)     server_name_lost_cb,
		self,
		NULL);
	if (!self->priv->bus_id)
	{
		g_warning(_("Unable to own bus name '%s'"), bus_name);
		g_free(bus_name);
		goto complete_setup_error;
	}
	g_free(bus_name);

	return;

complete_setup_error:

	gel_free_and_invalidate(error, NULL, g_error_free);
	gel_free_and_invalidate(self->priv->bus_id, 0, g_bus_unown_name);

	if (self->priv->player_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->player_id);
		self->priv->player_id = 0;
	}
	if (self->priv->root_id)
	{
		g_dbus_connection_unregister_object(self->priv->conn, self->priv->root_id);
		self->priv->root_id = 0;
	}

	gel_free_and_invalidate(self->priv->nodeinfo, NULL, g_dbus_node_info_unref);
	gel_free_and_invalidate(self->priv->conn,     NULL, g_object_unref);
}

#if 0
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
#endif

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
	
	g_return_if_fail(self->priv->lomo == NULL);

	self->priv->lomo = g_object_ref(lomo);

	if (self->priv->lomo && self->priv->bus_name_suffix)
		complete_setup(self);

	g_object_notify((GObject *) self, "lomo-player");
}

const gchar *
eina_mpris_player_get_object_path(EinaMprisPlayer *self)
{
	g_return_val_if_fail(EINA_IS_MPRIS_PLAYER(self), NULL);
	return self->priv->bus_name_suffix;
}

static void
set_bus_name_suffix(EinaMprisPlayer *self, const gchar *bus_name_suffix)
{
	g_return_if_fail(EINA_IS_MPRIS_PLAYER(self));
	g_return_if_fail(bus_name_suffix != NULL);

	g_return_if_fail(self->priv->bus_name_suffix == NULL);

	self->priv->bus_name_suffix = g_strdup(bus_name_suffix);

	if (self->priv->lomo && self->priv->bus_name_suffix)
		complete_setup(self);

	g_object_notify((GObject *) self, "bus-name-suffix");
}

#if 0
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
#endif

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_warning("%s", __FUNCTION__);
}

static void
server_name_lost_cb (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	g_warning("%s", __FUNCTION__);
}

/**
 * Section: DBus access functions
 */
static void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	EinaMprisPlayer *self)
{
	g_warning("%s.%s", interface_name, method_name);

	if (g_str_equal("Quit", method_name) ||
		g_str_equal("Raise", method_name))
	{
		g_dbus_method_invocation_return_value(invocation, NULL);
		return;
	}
	else
		goto root_method_call_cb_error;

root_method_call_cb_error:
	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

static GVariant*
root_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self)
{
	g_warning("%s.%s", interface_name, property_name);

	if (g_str_equal("CanQuit", property_name) ||
		g_str_equal("CanRaise", property_name))
	{
		return g_variant_new_boolean(TRUE);
	}

	else if (g_str_equal("HasTrackList", property_name))
	{
		return g_variant_new_boolean(FALSE);
	}

	else if (g_str_equal("Identity", property_name))
	{
		return g_variant_new_string("Eina Music Player");
	}

	else if (g_str_equal("DesktopEntry", property_name))
	{
		return g_variant_new_string("eina.desktop");
	}

	else if (g_str_equal("SupportedUriSchemes", property_name))
	{
		const char *fake_supported_schemes[] = { "file", "http", "cdda", "smb", "sftp", NULL };
		return g_variant_new_strv (fake_supported_schemes, -1);
	}

	else if (g_str_equal(property_name, "SupportedMimeTypes"))
	{
		const char *fake_supported_mimetypes[] = { "application/ogg", "audio/x-vorbis+ogg", "audio/x-flac", "audio/mpeg", NULL };
		return g_variant_new_strv (fake_supported_mimetypes, -1);
	}

	else
		goto root_get_property_cb_error;

root_get_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static gboolean
root_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self)
{
	g_warning("%s.%s", interface_name, property_name);
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

static void
player_method_call_cb(GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   EinaMprisPlayer *self)

{
	g_warning("%s.%s", interface_name, method_name);
	LomoPlayer *lomo = self->priv->lomo;

	if (g_str_equal("PlayPause", method_name))
	{
		lomo_player_set_state(lomo,
			(lomo_player_get_state(lomo) == LOMO_STATE_PLAY) ? LOMO_STATE_PAUSE : LOMO_STATE_PLAY, NULL);
		return;
	}

	else if (g_str_equal("Next", method_name))
	{
		lomo_player_go_next(lomo, NULL);
		return;
	}
	
	else if (g_str_equal("Previous", method_name))
	{
		lomo_player_go_previous(lomo, NULL);
		return;
	}

	const gchar *nulls[] = { "Pause", "Play", "Stop", "Seek", "SetPosition", "OpenUri" };
	for (guint i = 0; i < G_N_ELEMENTS(nulls); i++)
	{
		if (g_str_equal(nulls[i], method_name))
		{
			g_dbus_method_invocation_return_value(invocation, NULL);
			return;
		}
	}

	goto player_method_call_cb_error;

player_method_call_cb_error:
	g_dbus_method_invocation_return_error (invocation,
		G_DBUS_ERROR,
		G_DBUS_ERROR_NOT_SUPPORTED,
		"Method %s.%s not supported",
		interface_name,
		method_name);
}

static GVariant *
player_get_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GError **error,
	EinaMprisPlayer *self)
{
	g_warning("%s.%s", interface_name, property_name);

	const gchar *bools[] = { "Shuffle", "CanGoNext", "CanGoPrevious", "CanPlay", "CanPause", "CanSeek", "CanControl" };
	for (guint i = 0; i < G_N_ELEMENTS(bools); i++)
		if (g_str_equal(bools[i], property_name))
			return g_variant_new_boolean(TRUE);

	if (g_str_equal("PlaybackStatus", property_name))
	{
		const gchar *ret = "Stopped";
		switch (lomo_player_get_state(self->priv->lomo))
		{
		case LOMO_STATE_PLAY:
			ret = "Playing";
			break;
		case LOMO_STATE_PAUSE:
			ret = "Paused";
			break;
		default:
			break;
		}
		return g_variant_new_string(ret);
	}

	if (g_str_equal("LoopStatus", property_name))
		return g_variant_new_string("None");

	if (g_str_equal("Rate", property_name))
		return g_variant_new_double(1.0);

	if (g_str_equal("Metadata", property_name))
	{
		GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
		g_variant_builder_end (builder);
	}

	if (g_str_equal("Volume", property_name))
		return g_variant_new_double(1.0);

	if (g_str_equal("Position", property_name))
		return g_variant_new_int64(0);

	if (g_str_equal("MinimumRate", property_name) ||
		g_str_equal("MaximumRate", property_name))
		return g_variant_new_double(1.0);

	goto player_get_property_cb_error;

player_get_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static gboolean
player_set_property_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *property_name,
	GVariant *value,
	GError **error,
	EinaMprisPlayer *self)
{
	g_warning("%s.%s", interface_name, property_name);
	
	const gchar *props[] = { "LoopStatus", "Rate", "Shuffle", "Volume" };
	for (guint i = 0; i < G_N_ELEMENTS(props); i++)
		if (g_str_equal(props[i], property_name))
			return TRUE;

	goto player_set_property_cb_error;

player_set_property_cb_error:
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

