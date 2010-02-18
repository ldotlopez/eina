/*
 * eina/dbus.c
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
#define GEL_DOMAIN "Eina::DBus"
#define EINA_PLUGIN_DATA_TYPE DBusGProxy
#include <config.h>
#include <eina/dbus.h>
#include <eina/eina-plugin.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <eina/dbus-marshallers.h>
#include <eina/lomo-player-dbus-glue.h>

/*
 * dbus-send --session --type=method_call --print-reply
 * --dest=org.gnome.SettingsDaemon /org/gnome/SettingsDaemon/MediaKeys
 *  org.gnome.SettingsDaemon.MediaKeys.GrabMediaPlayerKeys
 *
 *   dbus-send --session --type=method_call --print-reply
 *   --dest=org.gnome.SettingsDaemon /org/gnome/SettingsDaemon/MediaKeys
 *   org.freedesktop.DBus.Introspectable.Introspect
 */

GEL_AUTO_QUARK_FUNC(eina_dbus)

static void
key_pressed
(DBusGProxy *proxy, const gchar *application, const gchar *key, GelApp *app)
{
	if (!g_str_equal(application, PACKAGE_NAME))
		return;
	
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	if (lomo == NULL)
	{
		gel_error("No lomo available, ignoring key");
		return;
	}

	GError *error = NULL;
	if (g_str_equal(key, "Play") || g_str_equal(key, "Pause"))
	{
		LomoState state = lomo_player_get_state(lomo);
		if ((state == LOMO_STATE_PAUSE) || (state == LOMO_STATE_STOP))
			lomo_player_play(lomo, &error);
		else
			lomo_player_pause(lomo, &error);
	}

	else if (g_str_equal(key, "Next"))
		lomo_player_go_next(lomo, &error);
	else if (g_str_equal(key, "Previous"))
		lomo_player_go_prev(lomo, &error);
	else
	{
		gel_warn("Unknow command: %s", key);
		return;
	}

	if (error != NULL)
	{
		gel_warn("Cannot execute command %s: %s", key, error->message);
		g_error_free(error);
	}
}

static gboolean
dbus_register_server(GelApp *app, DBusGConnection *conn, GError **error)
{
	DBusGProxy *proxy = dbus_g_proxy_new_for_name(conn,
		DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS,
		DBUS_INTERFACE_DBUS);
	if (proxy == NULL)
	{
		g_set_error(error, eina_dbus_quark(), EINA_DBUS_CANNOT_GET_DBUS_PROXY,
			N_("Cannot get DBus proxy"));
		return FALSE;
	}

	guint result;
	org_freedesktop_DBus_request_name (proxy,
		"net.sourceforge.Eina.LomoPlayer", 
		DBUS_NAME_FLAG_DO_NOT_QUEUE, &result, error);
	if (*error)
	{
		g_object_unref(proxy);
		return FALSE;
	}

	LomoPlayer *lomo = gel_app_get_lomo(app);
	dbus_g_object_type_install_info(G_OBJECT_TYPE(lomo), &dbus_glib_lomo_player_object_info);
	dbus_g_connection_register_g_object (conn, "/net/sourceforge/Eina", (GObject*) lomo);
	g_object_unref(proxy);
	return TRUE;
}

static gboolean
dbus_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
 	DBusGConnection *connection = dbus_g_bus_get(DBUS_BUS_SESSION, error);
	if (connection == NULL)
		return FALSE;

	GError *err = NULL;
	if (!dbus_register_server(app, connection, &err))
	{
		gel_warn(N_("Cannot register DBus server: %s"), err->message);
		g_error_free(err);
	}

	DBusGProxy *proxy = dbus_g_proxy_new_for_name_owner(connection,
		"org.gnome.SettingsDaemon",
		"/org/gnome/SettingsDaemon/MediaKeys",
		"org.gnome.SettingsDaemon.MediaKeys",
		error);
	if (proxy == NULL)
	{
		GError *e = *error;
		if (e != NULL && (e->domain == DBUS_GERROR) && (e->code == DBUS_GERROR_UNKNOWN_METHOD))
		{
			g_clear_error(error);
			g_object_unref(proxy);

			proxy = dbus_g_proxy_new_for_name_owner(connection,
				"org.gnome.SettingsDaemon",
				"/org/gnome/SettingsDaemon",
				"org.gnome.SettingsDaemon",
				error);
			if (proxy == NULL)
				return FALSE;
		}
		else
			return FALSE;
	}

	dbus_g_proxy_call(proxy, "GrabMediaPlayerKeys", error,
		G_TYPE_STRING, PACKAGE_NAME,
		G_TYPE_UINT, 0,
		G_TYPE_INVALID,
		G_TYPE_INVALID);

	if (*error != NULL)
		return FALSE;

	dbus_g_object_register_marshaller (eina_dbus_marshal_VOID__STRING_STRING,
		G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_add_signal (proxy,
		"MediaPlayerKeyPressed",
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal (proxy,
		"MediaPlayerKeyPressed",
		G_CALLBACK (key_pressed),
		app, NULL);
	
	plugin->data = proxy;

	return TRUE;
}

static gboolean
dbus_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	DBusGProxy *proxy = EINA_PLUGIN_DATA(plugin);
	if (!dbus_g_proxy_call (proxy,
		"ReleaseMediaPlayerKeys", error,
		G_TYPE_STRING, PACKAGE_NAME,
		G_TYPE_INVALID, G_TYPE_INVALID))
		return FALSE;

	g_object_unref(proxy);
	return TRUE;
}

EINA_PLUGIN_SPEC(dbus,
	NULL,	// version
	"lomo",	// deps
	NULL,	// author
	NULL,	// url
	N_("DBus interface for Eina"),
	NULL,
	NULL,
	dbus_init, 
	dbus_fini);

