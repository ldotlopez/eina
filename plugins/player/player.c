/*
 * plugins/player/plugin.c
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

#include "eina/eina-plugin2.h"
#include "eina-player.h"
#include "eina/fs.h"

#define EINA_PLAYER_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.player"
#define EINA_PLAYER_STREAM_MARKUP_KEY  "stream-markup"
#define BUGS_URI ""
#define HELP_URI ""

static gboolean
action_activated_cb(EinaPlayer *player, GtkAction *action, GelPluginEngine *engine);
static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlayer *player);

G_MODULE_EXPORT gboolean
player_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GSettings *settings = gel_plugin_engine_get_settings(engine, EINA_PLAYER_PREFERENCES_DOMAIN);
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");

	GtkWidget *player = eina_player_new();
	g_object_set(player,
		"lomo-player",   gel_plugin_engine_get_interface(engine, "lomo"),
		"stream-markup", g_settings_get_string(settings, EINA_PLAYER_STREAM_MARKUP_KEY),
		NULL);

	// Connect actions
	g_signal_connect(player,   "action-activated", (GCallback) action_activated_cb, engine);
	g_signal_connect(settings, "changed",          (GCallback) settings_changed_cb, player);

	// Export
	gel_plugin_engine_set_interface(engine, "player", player);

	// Pack and show
	gtk_box_pack_start (
		(GtkBox *) gel_ui_application_get_window_content_area(application),
		player,
		FALSE, FALSE, 0);
	gtk_widget_show_all(player);

	return TRUE;
}

G_MODULE_EXPORT gboolean
player_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

static gboolean
action_activated_cb(EinaPlayer *player, GtkAction *action, GelPluginEngine *engine)
{
	const gchar *name = gtk_action_get_name(action);

	LomoPlayer *lomo = eina_player_get_lomo_player(player);
	g_return_val_if_fail(LOMO_IS_PLAYER(lomo), FALSE);

	GError *error = NULL;

	// Handle action
	if (g_str_equal(name, "open-action"))
		eina_fs_load_from_default_file_chooser(engine);

	// Action is unknow to us, return FALSE
	else
		return FALSE;

	// Show errors if any
	if (error != NULL)
	{
		g_warning(N_("Unable to complete action '%s': %s"), name, error->message);
		g_error_free(error);
	}
	// Successful or not, we try to handle it, return TRUE
	return TRUE;
}

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaPlayer *player)
{
	if (g_str_equal(key, EINA_PLAYER_STREAM_MARKUP_KEY))
	{
		eina_player_set_stream_markup(player, g_settings_get_string(settings, key));
	}

	else
	{
		g_warning(N_("Unhanded key '%s'"), key);
	}
}

EINA_PLUGIN_INFO_SPEC(player,
	NULL,
	"lomo",
	NULL,
	NULL,
	N_("Player plugin"),
	NULL,
	NULL
	);
