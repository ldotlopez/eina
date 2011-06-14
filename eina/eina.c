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
#include <lomo/lomo.h>
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
application_save_plugin_list_in_change(EinaApplication *app, PeasPluginInfo *info, gboolean include_plugin)
{
	return ;
	#if 0
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(GEL_IS_PLUGIN(plugin));

	GelPluginEngine *engine = application_get_plugin_engine(app);
	g_return_if_fail(GEL_IS_PLUGIN_ENGINE(engine));

	GList *visible = NULL;

	GList *current_plugins = gel_plugin_engine_get_plugins(engine);
	GList *l = current_plugins;
	while (l)
	{
		GelPlugin *_plugin = GEL_PLUGIN(l->data);
		const GelPluginInfo *info = gel_plugin_get_info(_plugin);
		if (info->hidden)
		{
			l = l->next;
			continue;
		}

		if ((_plugin == plugin) && !include_plugin)
		{
			l = l->next;
			continue;
		}

		visible = g_list_prepend(visible, g_strdup(info->name));
		l = l->next;
	}
	gel_free_and_invalidate(current_plugins, NULL, g_list_free);

	if (visible)
	{
		visible = g_list_reverse(visible);
		gchar **visible_strv = gel_list_to_strv(visible, FALSE);
		g_list_free(visible);

		g_settings_set_strv(eina_application_get_settings(app, EINA_DOMAIN), "plugins", (const gchar * const*) visible_strv);
		g_strfreev(visible_strv);
	}
	#endif
}

static void
engine_load_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *app)
{
	application_save_plugin_list_in_change(app, info, TRUE);
}

static void
engine_unload_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *app)
{
	application_save_plugin_list_in_change(app, info, TRUE);
}

static void
extension_set_extension_added_cb(PeasExtensionSet *set,
	PeasPluginInfo   *info,
	PeasExtension    *exten,
	EinaApplication  *application)
{
	GError *error = NULL;
	if (!eina_activatable_activate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to activate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
}

static void
extension_set_extension_removed_cb(PeasExtensionSet *set,
	PeasPluginInfo   *info,
	PeasExtension    *exten,
	EinaApplication  *application)
{
	GError *error = NULL;
	if (!eina_activatable_deactivate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to deactivate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
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
	activated = TRUE;

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	eina_stock_init();

	g_irepository_prepend_search_path("./gel");
	g_irepository_prepend_search_path("./lomo");
	g_irepository_prepend_search_path("./eina");

	const gchar *g_ir_reqs[] = {
		"Peas", "1.0",
		"Eina", "0.12",
		NULL
		};

	GError *error = NULL;
	GIRepository *repo = g_irepository_get_default();
	for (guint i = 0; g_ir_reqs[i] != NULL; i = i + 2)
	{
		if (!g_irepository_require(repo, g_ir_reqs[i], g_ir_reqs[i+1], G_IREPOSITORY_LOAD_FLAG_LAZY, &error))
		{
			g_warning(N_("Unable to load typelib %s %s: %s"), g_ir_reqs[i], g_ir_reqs[i+1], error->message);
			g_error_free(error);
			return;
		}
	}

	PeasEngine *engine = peas_engine_get_default();
	peas_engine_add_search_path(engine, g_getenv("EINA_LIB_PATH"), g_getenv("EINA_LIB_PATH"));
		
	application_set_plugin_engine(EINA_APPLICATION(application), engine);

	// gchar  *req_plugins[] = { "dbus", "player", "playlist", NULL };
	gchar  *req_plugins[] = { "player", NULL };

	//gchar **opt_plugins = g_settings_get_strv(
	//		eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN),
	//		"plugins");
	gchar **opt_plugins = NULL;

	gchar **plugins = gel_strv_concat(
		req_plugins,
		opt_plugins,
		NULL);
	g_strfreev(opt_plugins);

	// ExtensionSet
	PeasExtensionSet *es = peas_extension_set_new (engine,
		EINA_TYPE_ACTIVATABLE,
		NULL);

	g_signal_connect(es, "extension-added",   G_CALLBACK(extension_set_extension_added_cb),   application);
	g_signal_connect(es, "extension-removed", G_CALLBACK(extension_set_extension_removed_cb), application);

	guint  n_plugins = g_strv_length(plugins);
	guint  i;
	for (i = 0; i < n_plugins; i++)
	{
		// GError *error = NULL;
		PeasPluginInfo *info = peas_engine_get_plugin_info(engine, plugins[i]);
		if (!info || !peas_engine_load_plugin(engine, info))
		// if (!gel_plugin_engine_load_plugin_by_name(engine, plugins[i], &error))
		{
			// g_warning(N_("Unable to load required plugin '%s': %s"), plugins[i], error->message);
			// g_error_free(error);
			g_warning(N_("Unable to load required plugin '%s'"), plugins[i]);
		}
	}
	g_strfreev(plugins);

	// g_signal_connect(engine, "plugin-init", (GCallback) engine_plugin_init_cb, application);
	// g_signal_connect(engine, "plugin-fini", (GCallback) engine_plugin_fini_cb, application);
	g_signal_connect(engine, "load-plugin",   (GCallback) engine_load_plugin_cb,   application);
	g_signal_connect(engine, "unload-plugin", (GCallback) engine_unload_plugin_cb, application);

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

	LomoPlayer *lomo = LOMO_PLAYER(eina_application_get_lomo(EINA_APPLICATION(application)));

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
		eina_fs_load_from_playlist(EINA_APPLICATION(application), playlist);
		g_free(playlist);
	}

	return 0;
}

static void
app_open_cb(GApplication *application, gpointer files, gint n_files, gchar *hint, gpointer data)
{
	eina_fs_load_files_multiple(EINA_APPLICATION(application), files, n_files);
}

gint main(gint argc, gchar *argv[])
{
	g_type_init();
	gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	gtk_init(&argc, &argv);

	for (guint i = 0; i < argc; i++)
	{
		if (g_str_has_prefix(argv[i], "--introspect-dump="))
		{
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
	g_signal_handlers_disconnect_by_func(engine, (GCallback) engine_load_plugin_cb, app);
	g_signal_handlers_disconnect_by_func(engine, (GCallback) engine_unload_plugin_cb, app);
	g_object_unref(engine);
	g_object_unref(app);

	// Fuc*** gcc issues
	if (app == (EinaApplication *)0xdeadbeef)
	{
		eina_fs_load_from_default_file_chooser(NULL);
	}

	return status;
}

