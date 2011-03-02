#include <eina/eina-plugin.h>
#include <gel/gel.h>
#include <glib/gi18n.h>

#define ENTRY_OBJECT_PATH_PREFIX 	"/org/mpris/MediaPlayer2/Track/"
#include "mpris-spec.h"

typedef struct {
	GDBusConnection *connection;
	GDBusNodeInfo *node_info;
	guint name_own_id, root_id, player_id;
} MprisPlugin;

static void
root_method_call_cb (GDBusConnection *connection,
	const char *sender,
	const char *object_path,
	const char *interface_name,
	const char *method_name,
	GVariant *parameters,
	GDBusMethodInvocation *invocation,
	MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
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
	MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return NULL;
}

static const GDBusInterfaceVTable root_vtable =
{
	(GDBusInterfaceMethodCallFunc) root_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) root_get_property_cb,
	NULL
};

static void
player_method_call_cb(GDBusConnection *connection,
			   const char *sender,
			   const char *object_path,
			   const char *interface_name,
			   const char *method_name,
			   GVariant *parameters,
			   GDBusMethodInvocation *invocation,
			   MprisPlugin *plugin)

{
	g_warning("%s", __FUNCTION__);
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
	MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
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
	MprisPlugin *plugin)
{
	g_warning("%s", __FUNCTION__);
	g_set_error (error,
		     G_DBUS_ERROR,
		     G_DBUS_ERROR_NOT_SUPPORTED,
		     "Property %s.%s not supported",
		     interface_name,
		     property_name);
	return FALSE;
}

static const GDBusInterfaceVTable player_vtable =
{
	(GDBusInterfaceMethodCallFunc)  player_method_call_cb,
	(GDBusInterfaceGetPropertyFunc) player_get_property_cb,
	(GDBusInterfaceSetPropertyFunc) player_set_property_cb
};

static void
name_acquired_cb (GDBusConnection *connection, const char *name, MprisPlugin *plugin)
{
	g_warning ("successfully acquired dbus name %s", name);
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
		MPRIS_OBJECT_NAME,
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
		MPRIS_OBJECT_NAME,
		ifaceinfo,
		&player_vtable,
		plugin,
		NULL,
		&err)))
	{
		g_warning ("Unable to register MPRIS player interface: %s", err->message);
		g_error_free (err);
	}

	_plugin->name_own_id = g_bus_own_name (G_BUS_TYPE_SESSION,
		MPRIS_BUS_NAME_PREFIX ".eina",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		NULL,
		(GBusNameAcquiredCallback) name_acquired_cb,
		(GBusNameLostCallback) name_lost_cb,
		_plugin,
		NULL);

	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}
