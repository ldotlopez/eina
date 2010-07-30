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

#define BUGS_URI ""
#define HELP_URI ""

static void
action_activated_cb(GtkAction *action, GelPluginEngine *engine);

G_MODULE_EXPORT gboolean
player_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	GelUIApplication *application = gel_plugin_engine_get_interface(engine, "application");
	GtkVBox *content = gel_ui_application_get_window_content_area(application);

	GtkWidget *player = eina_player_new();
	gtk_box_pack_start ((GtkBox *) content, player, TRUE, TRUE, 0);
	gtk_widget_show_all(player);

	// Lomo
	eina_player_set_lomo_player ((EinaPlayer *) player, gel_plugin_engine_get_interface(engine, "lomo"));

	// Connect actions
	GtkBuilder *builder = gel_ui_generic_get_builder((GelUIGeneric *) player);
	const gchar *actions[] = {
		"prev-action",
		"next-action",
		"play-action",
		"pause-action",
		"open-action"
		};
	for (guint i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GtkAction *action = NULL;
		if (!(action = GTK_ACTION(gtk_builder_get_object(builder, actions[i]))))
			g_warning(N_("Action '%s' not found in widget"), actions[i]);
		else
			g_signal_connect(action, "activate", (GCallback) action_activated_cb, engine);
	}
	gel_plugin_engine_set_interface(engine, "player", player);
	
	return TRUE;
}

G_MODULE_EXPORT gboolean
player_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

static void
action_activated_cb(GtkAction *action, GelPluginEngine *engine)
{
	const gchar *name = gtk_action_get_name(action);

	LomoPlayer *lomo = gel_plugin_engine_get_interface(engine, "lomo");
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	GError *error = NULL;
	if (g_str_equal(name, "play-action"))
		lomo_player_play(lomo, &error);

	else if (g_str_equal(name, "pause-action"))
		lomo_player_pause(lomo, &error);

	else if (g_str_equal(name, "next-action"))
		lomo_player_go_next(lomo, &error);

	else if (g_str_equal(name, "prev-action"))
		lomo_player_go_prev(lomo, &error);

	else if (g_str_equal(name, "open-action"))
		eina_fs_load_from_default_file_chooser(engine);

	else if (g_str_equal(name, "quit-action"))
		gtk_application_quit(gel_plugin_engine_get_interface(engine, "application"));

	else if (g_str_equal(name, "help-action"))
#if OSX_SYSTEM
		g_spawn_command_line_async(OSX_OPEN_PATH " " HELP_URI, &error);
#else
		gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, &error);
#endif

	else if (g_str_equal(name, "bug-action"))
#if OSX_SYSTEM
		g_spawn_command_line_async(OSX_OPEN_PATH " " BUGS_URI, &error);
#else
		gtk_show_uri(NULL, BUGS_URI, GDK_CURRENT_TIME, &error);
#endif
#if 0
	else if (g_str_equal(name, "about-action"))
		eina_about_show(eina_obj_get_about((EinaObj *) self));
#endif
	else
	{
		g_warning(N_("Unknow action: %s"), name);
		return;
	}

	if (error != NULL)
	{
		g_warning(N_("Unable to complete action '%s': %s"), name, error->message);
		g_error_free(error);
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
