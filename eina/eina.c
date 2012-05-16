/*
 * eina/eina.c
 *
 * Copyright (C) 2004-2011 Eina
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#define DEBUG 1
#define DEBUG_PREFIX "EinaMain"
#if DEBUG
#define debug(...) g_debug(DEBUG_PREFIX " " __VA_ARGS__)
#else
#define debug(...) ;
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/lomo/eina-lomo-plugin.h>
#include "eina/ext/eina-application.h"
#include "eina/ext/eina-fs.h"
#include "eina/ext/eina-stock.h"

static void
app_open_cb(GApplication *application, gpointer files, gint n_files, gchar *hint, gpointer data)
{
	eina_fs_load_gfile_array(EINA_APPLICATION(application), files, n_files);
}

static gint
app_command_line_cb (EinaApplication *self, GApplicationCommandLine *command_line)
{
	gboolean opt_b_ign = FALSE;
	gboolean opt_play  = FALSE;
	gboolean opt_stop  = FALSE;
	gboolean opt_pause = FALSE;
	gboolean opt_clear = FALSE;
	gchar**  opt_uris  = NULL;
	const GOptionEntry opt_entries[] = {
		{ "new-instance",    'n', 0, G_OPTION_ARG_NONE,           &opt_b_ign, _("Don't reuse existing windows"), NULL },
		{ "play",            's', 0, G_OPTION_ARG_NONE,           &opt_play,  _("Set play state"), NULL },
		{ "stop",            'S', 0, G_OPTION_ARG_NONE,           &opt_stop,  _("Set stop state"), NULL },
		{ "pause",           'p', 0, G_OPTION_ARG_NONE,           &opt_pause, _("Set pause state"), NULL },
		{ "clear",           'c', 0, G_OPTION_ARG_NONE,           &opt_clear, _("Clear playlist (use it replace current playlist)"), NULL },
		{ G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_FILENAME_ARRAY, &opt_uris,   NULL, _("[FILES/URIs...]") },
		{ NULL }
	};

	GOptionContext *opt_ctx = g_option_context_new(NULL);
	g_option_context_set_help_enabled(opt_ctx, TRUE);
	g_option_context_add_main_entries(opt_ctx, opt_entries, GETTEXT_PACKAGE);

	gint    argc;
	gchar **argv = g_application_command_line_get_arguments (command_line, &argc);

	GError *err = NULL;
	if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
	{
		g_warning("Option parsing failed: %s", err->message);
		g_error_free(err);
		g_option_context_free(opt_ctx);
		g_strfreev (argv);
		return 0;
	}
	g_option_context_free(opt_ctx);
	g_strfreev (argv);

	LomoPlayer *lomo = LOMO_PLAYER(eina_application_get_interface(self, "lomo"));

	LomoState state = LOMO_STATE_INVALID;
	if (opt_play)  state = LOMO_STATE_PLAY;
	if (opt_stop)  state = LOMO_STATE_STOP;
	if (opt_pause) state = LOMO_STATE_PAUSE;
	if (state != LOMO_STATE_INVALID)
		lomo_player_set_state(lomo, state, NULL);

	if (opt_uris)
	{
		GFile **gfiles = g_new0(GFile*, g_strv_length(opt_uris));
		for (guint i = 0; opt_uris[i]; i++)
			gfiles[i] = g_file_new_for_commandline_arg(opt_uris[i]);
		g_application_open((GApplication *) self, gfiles, g_strv_length(opt_uris), "");
		for (guint i = 0; opt_uris[i]; i++)
			g_object_unref(gfiles[i]);
		g_free(gfiles);
		g_strfreev(opt_uris);
	}
	else
	{
		gchar *playlist = g_build_filename(g_get_user_config_dir(), PACKAGE, "playlist", NULL);
		eina_fs_load_playlist(self, playlist);
		g_free(playlist);
	}

	return 0;
}

gint main(gint argc, gchar *argv[])
{
	// Minimal bootstrap (gtype system and libgel)
	g_type_init();
	gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);

	// Small hack to enable introspection features
	for (guint i = 0; i < argc; i++)
	{
		if (g_str_has_prefix(argv[i], "--introspect-dump="))
		{
			PeasEngine *engine = eina_application_create_standalone_engine(TRUE);
			gchar *plugins[] = { "lomo", "preferences", "dock" , "playlist", "player",
			#if HAVE_SQLITE3
			"adb", "muine",
			#endif
			NULL };
			for (guint i = 0; plugins[i]; i++)
				peas_engine_load_plugin(engine, peas_engine_get_plugin_info(engine, plugins[i]));
			g_irepository_dump(argv[i] + strlen("--introspect-dump="), NULL);
			return 0;
		}
	}

	// i18n bootstrap
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	// UI init
	gtk_init(NULL, NULL);

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);

	eina_stock_init();

	// Pulse audio
	g_setenv("PULSE_PROP_media.role", "audio", TRUE);

	// Setup GI_TYPELIB_PATH
	gchar *tmp = g_build_filename(PACKAGE_PREFIX, "lib", "girepository-1.0", NULL);
	GSList *curr_paths = g_irepository_get_search_path();
	GSList *iter = curr_paths;
	while (iter && (g_strcmp0(tmp, (gchar *) iter->data) != 0))
		iter = iter->next;

	if (iter == NULL)
		g_irepository_prepend_search_path(tmp);
	g_free(tmp);

	// Setup GSETTINGS_SCHEMA_DIR
	tmp = g_build_filename(PACKAGE_PREFIX, "share", "glib-2.0", "schemas", NULL);
	g_setenv("GSETTINGS_SCHEMA_DIR", tmp, FALSE);
	g_free(tmp);

	// Misc stuff
	tmp = g_strdup_printf(_("%s music player"), PACKAGE_NAME);
	g_set_application_name(tmp);
	g_free(tmp);

	// Check for the -n o --new-instance argument to modify our application-id
	EinaApplication *app = NULL;
	if ((argc >= 2) && (g_str_equal("-n", argv[1]) || g_str_equal("--new-instance", argv[1])))
	{
		gchar *id = g_strdup_printf("%s-%u", EINA_APP_DOMAIN, g_random_int());
		app = eina_application_new(id);
	}
	else
		app = eina_application_new(EINA_APP_DOMAIN);

	g_settings_bind(eina_application_get_settings(EINA_APPLICATION(app), EINA_APP_DOMAIN), "prefer-dark-theme",
		G_OBJECT(gtk_settings_get_default()), "gtk-application-prefer-dark-theme",
		G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(app, "command-line", (GCallback) app_command_line_cb, NULL);
	g_signal_connect(app, "open",         (GCallback) app_open_cb, NULL);

	gint status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref(app);

	return status;
}

