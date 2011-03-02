#include <eina/eina-plugin.h>
#include <gel/gel.h>
#include <glib/gi18n.h>

#include "mpris.h"
#include "mpris-spec.h"
#include "bridge.h"

static void
name_acquired_cb (GDBusConnection *connection, const char *name, MprisPlugin *plugin)
{
	g_warning ("successfully acquired dbus name %s", name);
	if (!g_dbus_connection_send_message(plugin->connection,
			g_dbus_message_new_signal(MPRIS_OBJECT_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged"),
			0,
			NULL,
			NULL))
		g_warning(_("Error sending signal"));
}

static void
name_lost_cb (GDBusConnection *connection, const char *name, MprisPlugin *plugin)
{
	g_warning ("lost dbus name %s", name);
}

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new0(MprisPlugin, 1);
	GError *err = NULL;

	if (!(_plugin->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err)))
	{
		g_warning ("Unable to connect to D-Bus session bus: %s", err->message);
		return FALSE;
	}

	/* parse introspection data */
	if (!(_plugin->node_info = g_dbus_node_info_new_for_xml (mpris_introspection_xml, &err)))
	{
		g_warning ("Unable to read MPRIS interface specificiation: %s", err->message);
		return FALSE;
	}

	GDBusInterfaceInfo *ifaceinfo = NULL;

	/* register root interface */
	ifaceinfo = g_dbus_node_info_lookup_interface (_plugin->node_info, MPRIS_ROOT_INTERFACE);
	if (!(_plugin->root_id = g_dbus_connection_register_object (_plugin->connection,
		MPRIS_OBJECT_PATH,
		ifaceinfo,
		&root_vtable,
		plugin,
		NULL,
		&err)))
	{
		g_warning ("unable to register MPRIS root interface: %s", err->message);
		g_error_free (err);
	}

	/* register player interface */
	ifaceinfo = g_dbus_node_info_lookup_interface (_plugin->node_info, MPRIS_PLAYER_INTERFACE);
	if (!(_plugin->player_id = g_dbus_connection_register_object (_plugin->connection,
		MPRIS_OBJECT_PATH,
		ifaceinfo,
		&player_vtable,
		plugin,
		NULL,
		&err)))
	{
		g_warning ("Unable to register MPRIS player interface: %s", err->message);
		g_error_free (err);
	}

	_plugin->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
		MPRIS_BUS_NAME_PREFIX ".eina",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		(GBusAcquiredCallback)     NULL,
		(GBusNameAcquiredCallback) name_acquired_cb,
		(GBusNameLostCallback)     name_lost_cb,
		_plugin,
		NULL);

	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}
