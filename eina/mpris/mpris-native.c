#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "mpris.h"
#include "mpris-spec.h"
#include "bridge.h"

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);
static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin);

// GEL_DEFINE_QUARK_FUNC(mpris)

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new(MprisPlugin, 1);
	GError *err = NULL;

	_plugin->lomo = eina_application_get_lomo(app);

	// Connect to session bus
	_plugin->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);

	// Load introspection data
    _plugin->node_info = g_dbus_node_info_new_for_xml(mpris_introspection_xml, &err);

	// Register root 

	if (!(_plugin->root_id = g_dbus_connection_register_object(_plugin->connection,
		MPRIS_OBJECT_PATH,
		g_dbus_node_info_lookup_interface(_plugin->node_info, MPRIS_ROOT_INTERFACE),
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
	if (!(_plugin->player_id = g_dbus_connection_register_object(_plugin->connection,
		MPRIS_OBJECT_PATH,
		g_dbus_node_info_lookup_interface(_plugin->node_info, MPRIS_PLAYER_INTERFACE),
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
		MPRIS_BUS_NAME_PREFIX ".eina",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		NULL,
		(GBusNameAcquiredCallback) server_name_acquired_cb,
		(GBusNameLostCallback)     server_name_lost_cb,
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

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("=>%s: %d", name, g_dbus_connection_send_message(connection,
			g_dbus_message_new_signal(MPRIS_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged"),
			G_DBUS_SEND_MESSAGE_FLAGS_NONE,
			NULL,
			NULL));
}

static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
}

