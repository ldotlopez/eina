/*
 * eina/dbus/eina-dbus-plugin.c
 *
 * Based on: rb-mmkeys-plugin.c
 *
 *  Copyright (C) 2002, 2003 Jorn Baayen <jorn@nl.linux.org>
 *  Copyright (C) 2002,2003 Colin Walters <walters@debian.org>
 *  Copyright (C) 2007  James Livingston  <doclivingston@gmail.com>
 *  Copyright (C) 2007  Jonathan Matthew  <jonathan@d14n.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * The Rhythmbox authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Rhythmbox. This permission is above and beyond the permissions granted
 * by the GPL license by which Rhythmbox is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEBUG 0
#if DEBUG
#	define debug(...) g_debug(__VA_ARGS__)
#else
#	define debug(...) ;
#endif

#include <eina/ext/eina-extension.h>
#include <eina/lomo/eina-lomo-plugin.h>

#define EINA_TYPE_DBUS_PLUGIN         (eina_dbus_plugin_get_type ())
#define EINA_DBUS_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_DBUS_PLUGIN, EinaDBusPlugin))
#define EINA_DBUS_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_DBUS_PLUGIN, EinaDBusPluginClass))
#define EINA_IS_DBUS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_DBUS_PLUGIN))
#define EINA_IS_DBUS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_DBUS_PLUGIN))
#define EINA_DBUS_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_DBUS_PLUGIN, EinaDBusPluginClass))
typedef struct {
	GDBusProxy *proxy;
} EinaDBusPluginPrivate;
EINA_PLUGIN_REGISTER (EINA_TYPE_DBUS_PLUGIN, EinaDBusPlugin,
		      eina_dbus_plugin);

static void
media_player_key_pressed (GDBusProxy * proxy,
	const gchar *sender,
	const gchar *signal,
	GVariant * parameters, EinaDBusPlugin * plugin)
{
	gchar *key;
	gchar *application;

	if (g_str_equal(signal, "MediaPlayerKeyPressed"))
	{
		  g_warning
		      ("got unexpected signal '%s' from media player keys",
		       signal);
		  return;
	  }

	g_variant_get (parameters, "(ss)", &application, &key);

	debug ("got media key '%s' for application '%s'",
	       key, application);

	if (g_str_equal(application, PACKAGE_NAME))
	{
		g_warning ("Got media player key signal for unexpected application '%s'", application);
		return;
	}

	GError *error = NULL;
	EinaApplication *app = eina_activatable_get_application (EINA_ACTIVATABLE (plugin));
	LomoPlayer     *lomo = eina_application_get_lomo (app);

	if ((g_str_equal(key, "Play")) || g_str_equal(key, "Pause"))
	{
		LomoState state = lomo_player_get_state (lomo) == LOMO_STATE_PLAY ? LOMO_STATE_PAUSE : LOMO_STATE_PLAY;
		lomo_player_set_state (lomo, state, &error);
	}

	else if (strcmp (key, "Stop") == 0)
		lomo_player_set_state (lomo, LOMO_STATE_STOP, &error);

	else if (strcmp (key, "Previous") == 0)
		lomo_player_go_previous (lomo, &error);

	else if (strcmp (key, "Next") == 0)
		lomo_player_go_next (lomo, &error);

	else if ((strcmp (key, "Repeat") == 0) ||
	         (strcmp (key, "Random") == 0))
	{
		  const gchar *prop = g_str_equal(key, "Random") ? "random" : "repeat";
		  gboolean value;
		  g_object_get ((GObject *) lomo, prop, &value, NULL);
		  g_object_set ((GObject *) lomo, prop, !value, NULL);
	}
	/*
	   else if (strcmp (key, "FastForward") == 0) {
	   rb_shell_player_seek (plugin->shell_player, FFWD_OFFSET, NULL);
	   } else if (strcmp (key, "Rewind") == 0) {
	   rb_shell_player_seek (plugin->shell_player, -RWD_OFFSET, NULL);
	   }
	 */

	else
	{
		g_warning ("Unhandled media key: %s", key);
		return;
	}

	if (error != NULL)
	{
		g_warning ("Error handling media key '%s': %s", key, error->message);
		g_error_free (error);
	}

	g_free (key);
	g_free (application);
}

static void
grab_call_complete (GObject *proxy, GAsyncResult *res, EinaDBusPlugin *plugin)
{
	GError *error = NULL;
	GVariant *result;

	result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);
	if (error != NULL)
	{
		g_warning ("Unable to grab media player keys: %s", error->message);
		g_clear_error (&error);
	}
	else
		g_variant_unref (result);
}

static gboolean
window_focus_cb (GtkWidget *window, GdkEventFocus *event, EinaDBusPlugin *plugin)
{
	debug ("window got focus, re-grabbing media keys");

	g_dbus_proxy_call (plugin->priv->proxy,
		"GrabMediaPlayerKeys",
		g_variant_new ("(su)", PACKAGE_NAME, 0),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		(GAsyncReadyCallback) grab_call_complete,
		plugin);
	return FALSE;
}

static void
first_call_complete (GObject *proxy, GAsyncResult *res, EinaDBusPlugin *plugin)
{
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);
	if (error != NULL)
	{
		g_warning ("Unable to grab media player keys: %s", error->message);
		g_clear_error (&error);
		return;
	}

	debug ("grabbed media player keys");

	g_signal_connect_object (plugin->priv->proxy, "g-signal", G_CALLBACK (media_player_key_pressed), plugin, 0);

	/* re-grab keys when the main window gains focus */
	EinaApplication *app = eina_activatable_get_application (EINA_ACTIVATABLE (plugin));
	GObject      *window = G_OBJECT (eina_application_get_window (app));
	g_signal_connect_object (window, "focus-in-event", G_CALLBACK (window_focus_cb), plugin, 0);

	g_variant_unref (result);
}

static gboolean
eina_dbus_plugin_activate (EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	GDBusConnection *bus;
	EinaDBusPlugin *plugin;
	GError *error2 = NULL;

	debug ("activating media player keys plugin");

	plugin = EINA_DBUS_PLUGIN (activatable);

	bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error2);
	if (bus != NULL)
	{
		error2 = NULL;
		plugin->priv->proxy = g_dbus_proxy_new_sync (bus,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL,
			"org.gnome.SettingsDaemon",
			"/org/gnome/SettingsDaemon/MediaKeys",
			"org.gnome.SettingsDaemon.MediaKeys",
			NULL,
			&error2);
		if (error2 != NULL)
		{
			g_warning ("Unable to grab media player keys: %s", error2->message);
			g_clear_error (&error2);

		}
		else
		{
			g_dbus_proxy_call (plugin->priv->proxy,
				"GrabMediaPlayerKeys",
				g_variant_new ("(su)", PACKAGE_NAME, 0),
				G_DBUS_CALL_FLAGS_NONE, -1,
				NULL,
				(GAsyncReadyCallback)
				first_call_complete,
				plugin);
		}
	}
	else
	{
		g_warning ("couldn't get dbus session bus: %s", error2->message);
		g_clear_error (&error2);
	}

	return TRUE;
}

static void
final_call_complete (GObject * proxy, GAsyncResult * res, gpointer nothing)
{
	GError *error = NULL;
	GVariant *result;

	result = g_dbus_proxy_call_finish (G_DBUS_PROXY (proxy), res, &error);
	if (error != NULL)
	{
		  g_warning ("Unable to release media player keys: %s", error->message);
		  g_clear_error (&error);
	}
	else
	{
		g_variant_unref (result);
	}
}

static gboolean
eina_dbus_plugin_deactivate (EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	EinaDBusPlugin *plugin = EINA_DBUS_PLUGIN (activatable);

	if (plugin->priv->proxy != NULL)
	{
		g_dbus_proxy_call (plugin->priv->proxy,
			"ReleaseMediaPlayerKeys",
			g_variant_new ("(s)", PACKAGE_NAME),
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			(GAsyncReadyCallback)
			final_call_complete, NULL);
		gel_free_and_invalidate (plugin->priv->proxy, NULL, g_object_unref);
	}
	return TRUE;
}

