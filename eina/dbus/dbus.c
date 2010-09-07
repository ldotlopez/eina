#include <eina/eina-plugin2.h>
#include "dbus.h"
#include <eina/lomo/lomo.h>

#define BUS_NAME  "org.gnome.SettingsDaemon"
#define OBJECT    "/org/gnome/SettingsDaemon/MediaKeys"
#define INTERFACE "org.gnome.SettingsDaemon.MediaKeys"

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, GelPluginEngine *engine);

gboolean
dbus_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		BUS_NAME, OBJECT, INTERFACE,
		NULL,
		error
		);
	g_return_val_if_fail(proxy != NULL, FALSE);

	g_signal_connect (proxy, "g-signal", G_CALLBACK (proxy_signal_cb), engine);
	gel_plugin_set_data(plugin, proxy);

	return TRUE;
}

gboolean
dbus_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GDBusProxy *proxy = (GDBusProxy *) gel_plugin_steal_data(plugin);
	g_object_unref(proxy);
	return TRUE;
}

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, GelPluginEngine *engine)
{
	g_return_if_fail(g_str_equal("(ss)", (gchar *) g_variant_get_type(parameters)));

	GVariant *v_action = g_variant_get_child_value(parameters, 1);
	const gchar *action = g_variant_get_string(v_action, NULL);

	LomoPlayer *lomo = gel_plugin_engine_get_lomo(engine);
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
		g_warning(N_("Unhandled action: %s"), action);
}

