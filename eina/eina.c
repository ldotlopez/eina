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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/lomo/eina-lomo-plugin.h>
#include <libpeas/peas.h>
#include "eina/ext/eina-application.h"
#include "eina/ext/eina-activatable.h"
#include "eina/ext/eina-stock.h"
#include "eina/ext/eina-fs.h"

static PeasEngine*
application_get_plugin_engine(EinaApplication *app)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(app), NULL);
	return (PeasEngine *) g_object_get_data((GObject *) app, "x-eina-plugin-engine");
}

static void
application_set_plugin_engine(EinaApplication *app, PeasEngine *engine)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(PEAS_IS_ENGINE(engine));

	PeasEngine *test = application_get_plugin_engine(app);
	if (test != NULL)
		g_warning("EinaApplication object %p already has an PeasEngine", test);
	g_object_set_data((GObject *) app, "x-eina-plugin-engine", engine);
}

static void
extension_set_extension_added_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, EinaApplication *application)
{
	GError *error = NULL;
	if (!eina_activatable_activate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to activate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
}

static void
extension_set_extension_removed_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, EinaApplication *application)
{
	GError *error = NULL;
	if (!eina_activatable_deactivate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to deactivate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
}

static PeasEngine*
initialize_peas_engine(gboolean from_source)
{
	gchar *builddir = NULL;
	if (from_source)
	{
		g_type_init();
		gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	}

	// Setup girepository
	if (from_source)
	{
		builddir = g_path_get_dirname(g_get_current_dir());
		const gchar *subs[] = { "gel", "lomo", "eina", NULL };
		for (guint i = 0; subs[i]; i++)
		{
			gchar *tmp = g_build_filename(builddir, subs[i], NULL);
			g_debug("Add girepository search path: %s", tmp);
			g_irepository_prepend_search_path(tmp);
			g_free(tmp);
		}
	}

	const gchar *g_ir_req_full[] = { PACKAGE_NAME, EINA_API_VERSION, NULL };
	const gchar *g_ir_req_src[]  = { "Gel", GEL_API_VERSION, "Lomo", LOMO_API_VERSION, NULL };
	const gchar **g_ir_reqs = (const gchar **) (from_source ? g_ir_req_src : g_ir_req_full);

	GError *error = NULL;
	GIRepository *repo = g_irepository_get_default();
	for (guint i = 0; g_ir_reqs[i] != NULL; i = i + 2)
	{
		if (!g_irepository_require(repo, g_ir_reqs[i], g_ir_reqs[i+1], G_IREPOSITORY_LOAD_FLAG_LAZY, &error))
		{
			g_warning(N_("Unable to load typelib %s %s: %s"), g_ir_reqs[i], g_ir_reqs[i+1], error->message);
			g_error_free(error);
			return NULL;
		}
	}

	PeasEngine *engine = peas_engine_get_default();
	peas_engine_enable_loader (engine, "python");

	if (from_source)
	{
		gchar *plugins[] = { "lomo", "preferences", "dock" , "playlist", "player",
			#if HAVE_GTKMAC
			"osx",
			#endif
			#if HAVE_SQLITE3
			"adb", "muine",
			#endif
			NULL };
		for (guint i = 0; plugins[i]; i++)
		{
			gchar *tmp = g_build_filename(builddir, "eina", plugins[i], NULL);
			g_debug("Add PeasEngine search path: %s", tmp);
			peas_engine_add_search_path(engine, tmp, tmp);
			g_free(tmp);
		}
	}
	else
	{
		const gchar *libdir = NULL;

		if ((libdir = gel_get_package_lib_dir()) != NULL)
			peas_engine_add_search_path(engine, gel_get_package_lib_dir(), gel_get_package_lib_dir());

		if ((libdir = g_getenv("EINA_LIB_PATH")) != NULL)
			peas_engine_add_search_path(engine, g_getenv("EINA_LIB_PATH"), g_getenv("EINA_LIB_PATH"));

		gchar *user_plugin_path = NULL;

		#if defined OS_LINUX
		user_plugin_path = g_build_filename(g_get_user_data_dir(),
			PACKAGE, "plugins",
			NULL);

		#elif defined OS_OSX
		user_plugin_path = g_build_filename(g_get_home_dir(),
			"Library", "Application Support",
			PACKAGE_NAME, "Plugins",
			NULL);
		#endif

		if (user_plugin_path)
		{
			peas_engine_add_search_path(engine, user_plugin_path, user_plugin_path);
			g_free(user_plugin_path);
		}
	}

	gel_free_and_invalidate(builddir, NULL, g_free);

	return engine;
}

static void
app_activate_cb (GApplication *application, gpointer user_data)
{
	static gboolean activated = FALSE;
	if (activated)
	{
		GtkWindow *window = GTK_WINDOW(eina_application_get_window(EINA_APPLICATION(application)));
		if (!window || !GTK_IS_WINDOW(window))
			g_warn_if_fail(GTK_IS_WINDOW(window));
		gtk_window_present(window);
		return;
	}

	gtk_init(NULL, NULL);

	g_setenv("PULSE_PROP_media.role", "audio", TRUE);

	g_settings_bind(eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN), "prefer-dark-theme",
		G_OBJECT(gtk_settings_get_default()), "gtk-application-prefer-dark-theme",
		G_SETTINGS_BIND_DEFAULT);

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	eina_stock_init();

	PeasEngine *engine = initialize_peas_engine(FALSE);
	application_set_plugin_engine(EINA_APPLICATION(application), engine);

	gchar  *req_plugins[] = { "dbus", "player", "playlist",
		#ifdef HAVE_GTKMAC
		"osx",
		#endif
		NULL };

	gchar **opt_plugins = g_settings_get_strv(
			eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN),
			"plugins");

	gchar **plugins = gel_strv_concat(
		req_plugins,
		opt_plugins,
		NULL);
	g_strfreev(opt_plugins);

	// ExtensionSet
	PeasExtensionSet *es = peas_extension_set_new (engine,
		EINA_TYPE_ACTIVATABLE,
		"application", application,
		NULL);
	peas_extension_set_call(es, "activate", application, NULL);

	g_signal_connect(es, "extension-added",   G_CALLBACK(extension_set_extension_added_cb),   application);
	g_signal_connect(es, "extension-removed", G_CALLBACK(extension_set_extension_removed_cb), application);

	guint  n_req_plugins = g_strv_length(req_plugins);
	guint  n_plugins = g_strv_length(plugins);
	guint  i;
	for (i = 0; i < n_plugins; i++)
	{
		PeasPluginInfo *info = peas_engine_get_plugin_info(engine, plugins[i]);
		if (!info)
			g_warning(_("Unable to load plugin info"));

		// If plugin is hidden and is optional dont load.
		if (peas_plugin_info_is_hidden(info) && (i >= n_req_plugins))
			continue;

		if (!peas_engine_load_plugin(engine, info))
			g_warning(N_("Unable to load required plugin '%s'"), plugins[i]);
	}
	g_strfreev(plugins);
	activated = TRUE;

	g_settings_bind (
		eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN), "plugins",
		engine, "loaded-plugins",
		G_SETTINGS_BIND_SET);
	gtk_widget_show((GtkWidget *) eina_application_get_window((EinaApplication *) application));
}

static gint
app_command_line_cb (GApplication *application, GApplicationCommandLine *command_line, gpointer user_data)
{
	gboolean opt_play  = FALSE;
	gboolean opt_pause = FALSE;
	gboolean opt_clear = FALSE;
	gchar**  opt_uris  = NULL;
	const GOptionEntry opt_entries[] =
	{
		{ "play",            'p', 0, G_OPTION_ARG_NONE,           &opt_play,  _("Set play state"), NULL },
		{ "pause",           'x', 0, G_OPTION_ARG_NONE,           &opt_pause, _("Set stop state"), NULL },
		{ "clear",           'c', 0, G_OPTION_ARG_NONE,           &opt_clear, _("Clear playlist (use it replace current playlist)"), NULL },
		{ G_OPTION_REMAINING, 0,  0, G_OPTION_ARG_FILENAME_ARRAY, &opt_uris,   NULL, _("[FILES/URIs...]") },
		{ NULL }
	};

	// Handle arguments here
	gint argc;
	gchar **argv = g_application_command_line_get_arguments(command_line, &argc);

	GOptionContext *opt_ctx = g_option_context_new("Eina options");
	g_option_context_set_help_enabled(opt_ctx, TRUE);
	//  g_option_context_set_ignore_unknown_options(opt_ctx, TRUE); // Ignore unknow
	g_option_context_add_main_entries(opt_ctx, opt_entries, GETTEXT_PACKAGE);

	GError *err = NULL;
	if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
	{
		g_warning("Option parsing failed: %s", err->message);
		g_error_free(err);
		return 1;
	}
	g_option_context_free(opt_ctx);

	g_application_activate(application);

	LomoPlayer *lomo = LOMO_PLAYER(eina_application_get_interface(EINA_APPLICATION(application), "lomo"));

	if (opt_uris && !g_application_command_line_get_is_remote(command_line))
		opt_clear = TRUE;

	if (opt_play && !opt_uris)  lomo_player_play (lomo, NULL);
	if (opt_pause) lomo_player_pause(lomo, NULL);
	if (opt_clear) lomo_player_clear(lomo);

	if (opt_uris)
	{
		GFile **gfiles = g_new0(GFile*, g_strv_length(opt_uris));
		for (guint i = 0; opt_uris[i]; i++)
			gfiles[i] = g_file_new_for_commandline_arg(opt_uris[i]);

		g_application_open(application, gfiles, g_strv_length(opt_uris), "");
		for (guint i = 0; opt_uris[i]; i++)
			g_object_unref(gfiles[i]);
		g_free(gfiles);
		g_strfreev(opt_uris);

		if (opt_play)
			lomo_player_play (lomo, NULL);
	}

	if (!opt_uris && !g_application_command_line_get_is_remote(command_line))
	{
		gchar *playlist = g_build_filename(g_get_user_config_dir(), PACKAGE, "playlist", NULL);
		eina_fs_load_playlist(EINA_APPLICATION(application), playlist);
		g_free(playlist);
	}

	return 0;
}

static void
app_open_cb(GApplication *application, gpointer files, gint n_files, gchar *hint, gpointer data)
{
	eina_fs_load_gfile_array(EINA_APPLICATION(application), files, n_files);
}

gint main(gint argc, gchar *argv[])
{
	g_type_init();
	gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);

	for (guint i = 0; i < argc; i++)
	{
		if (g_str_has_prefix(argv[i], "--introspect-dump="))
		{
			PeasEngine *engine = initialize_peas_engine(TRUE);
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


	gchar *tmp = g_strdup_printf(_("%s music player"), PACKAGE_NAME);
	g_set_application_name(tmp);
	g_free(tmp);

	EinaApplication *app = eina_application_new(EINA_DOMAIN);
	g_signal_connect(app, "activate",     (GCallback) app_activate_cb, NULL);
	g_signal_connect(app, "command-line", (GCallback) app_command_line_cb, NULL);
	g_signal_connect(app, "open",         (GCallback) app_open_cb, NULL);

	gint status = g_application_run (G_APPLICATION (app), argc, argv);
	PeasEngine *engine = application_get_plugin_engine(app);
	g_object_unref(engine);
	g_object_unref(app);

	return status;
}

