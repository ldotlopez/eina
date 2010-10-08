#include <eina/eina-plugin2.h>
#include <eina/lomo/lomo.h>
#include "dbus.h"
#include "lomo-player-instropection.xml.h"

#define BUS_NAME  "org.gnome.SettingsDaemon"
#define OBJECT    "/org/gnome/SettingsDaemon/MediaKeys"
#define INTERFACE "org.gnome.SettingsDaemon.MediaKeys"

#define SERVER_BUS_NAME "net.sourceforge.Eina"
#define SERVER 0

typedef struct {
	guint server_id;
	GelPluginEngine *engine;
	GDBusProxy      *mmkeys_proxy;
} DBusData;

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, DBusData *data);
static void
server_bus_acquired_cb(GDBusConnection *connection, const gchar *name, DBusData *data);
static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, DBusData *data);
static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, DBusData *data);

gboolean
dbus_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	DBusData *data = g_new0(DBusData, 1);
	data->engine = engine;
	data->mmkeys_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		BUS_NAME, OBJECT, INTERFACE,
		NULL,
		error
		);
	g_warn_if_fail(data->mmkeys_proxy != NULL);
	if (data->mmkeys_proxy)
		g_signal_connect (data->mmkeys_proxy, "g-signal", G_CALLBACK (proxy_signal_cb), data);
	else
		g_warn_if_fail(data->mmkeys_proxy!= NULL);

	data->server_id = g_bus_own_name(G_BUS_TYPE_SESSION,
		SERVER_BUS_NAME,
		G_BUS_NAME_OWNER_FLAGS_NONE,
		(GBusAcquiredCallback) server_bus_acquired_cb,
		(GBusNameAcquiredCallback) server_name_acquired_cb,
		(GBusNameLostCallback) server_name_lost_cb,
		data,
		NULL);
	g_warn_if_fail(data->server_id > 0);

	gel_plugin_set_data(plugin, data);
	return TRUE;
}

gboolean
dbus_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	DBusData *data = (DBusData *) gel_plugin_steal_data(plugin);
	g_return_val_if_fail(data != NULL, FALSE);

	gel_free_and_invalidate(data->mmkeys_proxy, NULL, g_object_unref);
	gel_free_and_invalidate(data->server_id,    0,    g_bus_unown_name);
	return TRUE;
}

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, DBusData *data)
{
	g_return_if_fail(g_str_equal("(ss)", (gchar *) g_variant_get_type(parameters)));

	GVariant *v_action = g_variant_get_child_value(parameters, 1);
	const gchar *action = g_variant_get_string(v_action, NULL);

	LomoPlayer *lomo = gel_plugin_engine_get_lomo(data->engine);
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	if (g_str_equal("Play", action) || g_str_equal("Pause", action))
	{
		if (lomo_player_get_state(lomo) == LOMO_STATE_PLAY)
			lomo_player_pause(lomo, NULL);
		else
			lomo_player_play(lomo, NULL);
	}

	else if (g_str_equal("Next", action))
		lomo_player_go_next(lomo, NULL);

	else if (g_str_equal("Previous", action))
		lomo_player_go_prev(lomo, NULL);

	else
		g_warning(N_("Unknow action: %s"), action);
}

#if SERVER
static void
handle_method_call (GDBusConnection       *connection,
			              const gchar           *sender,
			              const gchar           *object_path,
			              const gchar           *interface_name,
			              const gchar           *method_name,
			              GVariant              *parameters,
			              GDBusMethodInvocation *invocation,
			              gpointer               user_data)
{
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
}

static GVariant *
handle_get_property (GDBusConnection  *connection,
			               const gchar      *sender,
			               const gchar      *object_path,
			               const gchar      *interface_name,
			               const gchar      *property_name,
			               GError          **error,
			               gpointer          user_data)
{
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
}

static gboolean
handle_set_property (GDBusConnection  *connection,
			               const gchar      *sender,
			               const gchar      *object_path,
			               const gchar      *interface_name,
			               const gchar      *property_name,
			               GVariant         *value,
			               GError          **error,
			               gpointer          user_data)
{
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
}

static const GDBusInterfaceVTable interface_vtable =
{
	handle_method_call,
	handle_get_property,
	handle_set_property
};
#endif

static void
server_bus_acquired_cb(GDBusConnection *connection, const gchar *name, DBusData *data)
{
	// g_warning("%s: %s", __FUNCTION__, name);
	GError *err = NULL;
	GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(lomo_player_instrospection_data, &err);
	if (!info)
	{
		g_warning("Unable to parse XML definition: %s", err->message);
		g_error_free(err);
		return;
	}
#if SERVER
	data->server_id = g_dbus_connection_register_object(connection,
		"/net/sourceforge/Eina/LomoPlayer",
		"net.sourceforge.Eina.Control"
		info->interfaces[0],
		&interface_vtable,
		NULL,
		&err);	
	if (!(data->server_id >= 0))
	{
		g_warning("Unable to register object: %s", err->message);
		g_error_free(err);
		return;
	}
#endif
}

static void
server_name_acquired_cb(GDBusConnection *connection, const gchar *name, DBusData *data)
{
	// g_warning("%s: %s", __FUNCTION__, name);
}

static void
server_name_lost_cb(GDBusConnection *connection, const gchar *name, DBusData *data)
{
	g_warning(N_("DBus server name %s lost"), name);
}

