/*
 * eina/dbus/dbus.c
 *
 * Copyright (C) 2004-2010 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>

#define BUS_NAME  "org.gnome.SettingsDaemon"
#define OBJECT    "/org/gnome/SettingsDaemon/MediaKeys"
#define INTERFACE "org.gnome.SettingsDaemon.MediaKeys"

typedef struct {
	guint server_id;
	EinaApplication *app;
	GDBusProxy      *mmkeys_proxy;
} DBusData;

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, DBusData *data);

gboolean
dbus_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	DBusData *data = g_new0(DBusData, 1);
	data->app = app;
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

	gel_plugin_set_data(plugin, data);
	return TRUE;
}

gboolean
dbus_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	DBusData *data = (DBusData *) gel_plugin_steal_data(plugin);
	g_return_val_if_fail(data != NULL, FALSE);

	gel_free_and_invalidate(data->mmkeys_proxy, NULL, g_object_unref);
	return TRUE;
}

static void
proxy_signal_cb(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, DBusData *data)
{
	g_return_if_fail(g_str_equal("(ss)", (gchar *) g_variant_get_type(parameters)));

	GVariant *v_action = g_variant_get_child_value(parameters, 1);
	const gchar *action = g_variant_get_string(v_action, NULL);

	LomoPlayer *lomo = eina_application_get_lomo(data->app);
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
		lomo_player_go_previous(lomo, NULL);

	else
		g_warning(N_("Unknow action: %s"), action);
}

