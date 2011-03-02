#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-mpris-introspection.h"

#define BUS_NAME         "org.mpris.MediaPlayer2.eina"
#define OBJECT_PATH      "/org/mpris/MediaPlayer2"
#define ROOT_INTERFACE   "org.mpris.MediaPlayer2"
#define PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

typedef struct {
	LomoPlayer *lomo;
	GDBusConnection *conn;
	guint bus_id, root_id, player_id;
} MprisPlugin;

enum {
	MPRIS_ERROR_NOT_IMPLEMENTED = 1
};

// Root interfaces
static void
root_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data);
static GVariant *
root_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data);
static gboolean
root_interface_set_property(GDBusConnection  *connection, 
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GVariant     *value,
	GError      **error,
	gpointer      user_data);

// Player interfaces
static void
player_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data);
static GVariant *
player_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data);
static gboolean
player_interface_set_property(GDBusConnection  *connection, 
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GVariant     *value,
	GError      **error,
	gpointer      user_data);

// Callbacks
static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);
static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);

static const GDBusInterfaceVTable root_vtable =
{
	root_interface_method_call,
	root_interface_get_property,
	root_interface_set_property
};
static const GDBusInterfaceVTable player_vtable =
{
	player_interface_method_call,
	player_interface_get_property,
	player_interface_set_property
};

GEL_DEFINE_QUARK_FUNC(mpris)

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new(MprisPlugin, 1);
	GError *err = NULL;

	_plugin->lomo = eina_application_get_lomo(app);

	// Connect to session bus
	_plugin->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);

	// Load introspection data
    GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(_eina_mpris_introspection, &err);

	// Register root 

	if (!(_plugin->root_id = g_dbus_connection_register_object(_plugin->conn,
		OBJECT_PATH,
		g_dbus_node_info_lookup_interface(info, ROOT_INTERFACE),
		&root_vtable,
		_plugin,
		NULL,
		&err)))
	{
		g_warning("Error with root interface: %s", err->message);
		g_error_free(err);
		err = NULL;
	}

	// Register player
	if (!(_plugin->player_id = g_dbus_connection_register_object(_plugin->conn,
		OBJECT_PATH,
		g_dbus_node_info_lookup_interface(info, PLAYER_INTERFACE),
		&player_vtable,
		_plugin,
		NULL,
		&err)))
	{
		g_warning("Error with root interface: %s", err->message);
		g_error_free(err);
		err = NULL;
	}
	
	if (!(_plugin->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
		BUS_NAME,
		G_BUS_NAME_OWNER_FLAGS_NONE,
		NULL,
		(GBusNameAcquiredCallback) server_name_acquired_cb,
		(GBusNameLostCallback) server_name_lost_cb,
		_plugin,
		NULL)))
	{
		g_warning("Error owning bus");
		g_error_free(err);
		err = NULL;
	}



	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

/*
 * Glue code for org.mpris.MediaPlayer2
 */
static void
root_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data)
{
	GError *error = NULL;
	g_warning("Call to %s", method_name);
	g_set_error(&error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	g_dbus_method_invocation_return_gerror(invocation, error);
	#if 0
	LomoPlayer *lomo = user_data;

	if (g_str_equal(method_name, "Play"))
	{
		/*
		gint change;
		g_variant_get (parameters, "(i)", &change);

		my_object_change_count (lomo, change);

		g_dbus_method_invocation_return_value (invocation, NULL);
		*/
		lomo_player_play(lomo);
		g_dbus_method_invocation_return_value(invocation, NULL);
	}
	#endif
}

static GVariant *
root_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to get %s", property_name);

	g_set_error(error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	return NULL;

	#if 0
	GVariant *ret;
	LomoPlayer *lomo = user_data;

	ret = NULL;
	if (g_strcmp0 (property_name, "Count") == 0)
		{
			ret = g_variant_new_int32 (lomo->count);
		}
	else if (g_strcmp0 (property_name, "Name") == 0)
		{
			ret = g_variant_new_string (lomo->name ? lomo->name : "");
		}

	return ret;
	#endif
}

static gboolean
root_interface_set_property(GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GVariant     *value,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to set %s", property_name);
	g_set_error(error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	return FALSE;

	#if 0
	LomoPlayer *lomo = user_data;

	if (g_strcmp0 (property_name, "Count") == 0)
		{
			g_object_set (lomo, "count", g_variant_get_int32 (value), NULL);
		}
	else if (g_strcmp0 (property_name, "Name") == 0)
		{
			g_object_set (lomo, "name", g_variant_get_string (value, NULL), NULL);
		}

	return TRUE;
	#endif
}

/*
 * Glue code for org.mpris.MediaPlayer2.Player
 */
static void
player_interface_method_call(GDBusConnection       *connection,
	const gchar           *sender,
	const gchar           *object_path,
	const gchar           *interface_name,
	const gchar           *method_name,
	GVariant              *parameters,
	GDBusMethodInvocation *invocation,
	gpointer               user_data)
{
	GError *error = NULL;
	g_warning("Call to %s", method_name);
	g_set_error(&error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	g_dbus_method_invocation_return_gerror(invocation, error);
}

static GVariant *
player_interface_get_property (GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to get %s", property_name);
	g_set_error(error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	return NULL;
}

static gboolean
player_interface_set_property(GDBusConnection  *connection,
	const gchar  *sender,
	const gchar  *object_path,
	const gchar  *interface_name,
	const gchar  *property_name,
	GVariant     *value,
	GError      **error,
	gpointer      user_data)
{
	g_warning("Call to set %s", property_name);
	g_set_error(error, mpris_quark(), MPRIS_ERROR_NOT_IMPLEMENTED, _("Not implemented"));
	return FALSE;
}

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("=>%s: %d", name, g_dbus_connection_send_message(connection,
			g_dbus_message_new_signal(OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged"),
			G_DBUS_SEND_MESSAGE_FLAGS_NONE,
			NULL,
			NULL));
}

static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
}

