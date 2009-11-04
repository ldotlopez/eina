/*
 * eina/v.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::Main"
#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gel/gel-io.h>
#include <gel/gel-io-ops.h>
#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>
#include <lomo/lomo-util.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>
#if HAVE_UNIQUE
#include <unique/unique.h>
#endif

static gint     opt_debug_level = GEL_DEBUG_LEVEL_WARN;
static gboolean opt_enqueue = FALSE;
static gboolean opt_new_instance = FALSE;
static gchar**  opt_uris = NULL;
static const GOptionEntry opt_entries[] =
{
	{ "debug-level",  'd', 0, G_OPTION_ARG_INT,  &opt_debug_level,  "Debug level", "<int 0..5>" },
	{ "enqueue",      'e', 0, G_OPTION_ARG_NONE, &opt_enqueue,      "Enqueue files instead of directly play", NULL},
	{ "new-instance", 'n', 0, G_OPTION_ARG_NONE, &opt_new_instance, "Ignore running instances and create a new one", NULL},

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opt_uris, NULL, "[FILE...]"},
	{ NULL }
};

#if HAVE_UNIQUE
// --
// UniqueApp stuff 
// --
enum
{
	COMMAND_0,
	COMMAND_PLAY,
	COMMAND_ENQUEUE
};

static UniqueResponse
unique_message_received_cb (UniqueApp *unique,
	UniqueCommand      command,
	UniqueMessageData *message,
	guint              time_,
	gpointer           data);
#endif


static void
open_uri_hook(GtkWidget *widget, const gchar *link_, gpointer user_data)
{
	GError *err = NULL;
	if (!gtk_show_uri(NULL, link_, GDK_CURRENT_TIME, &err))
	{
		gel_error("Cannot open link '%s': %s", link_, err->message);
		g_error_free(err);
	}
}

#ifdef DIAGNOSE_GDK_PIXBUF
static void
inspect_gdk_pixbuf_loaders(void)
{
	GSList *loaders = gdk_pixbuf_get_formats();
	GSList *iter = loaders;
	gel_warn("Detected %d Gdk-Pixbuf loaders", g_slist_length(loaders));
	while (iter)
	{
		gel_warn("Gdk-Pixbuf loader found: %s%s",
			gdk_pixbuf_format_get_name((GdkPixbufFormat *) iter->data),
			gdk_pixbuf_format_is_disabled((GdkPixbufFormat *) iter->data) ? "(disabled)" : ""
			);
		iter = iter->next;
	}
	g_slist_free(loaders);
}
#endif

// --
// Callbacks
// --
static void
list_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data);
static void
list_read_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);
static void
app_dispose_cb(GelApp *app, gpointer data);

// --
// XXX ugly hack
// --
void xxx_ugly_hack(void);

gint main
(gint argc, gchar *argv[])
{
	GelApp         *app;
#if HAVE_UNIQUE
	UniqueApp      *unique = NULL;
#endif
	gint            i = 0;
	gchar          *modules[] = { 
	// Modules loaded by default, keep in sync with eina-plugin.h include
	// headers
	"lomo", "settings",

	// "Top-level" plugins loaded by default
	"vogon",

	NULL };

	GOptionContext *opt_ctx;
	GError *err = NULL;

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);
	lomo_init(&argc, &argv);
	gel_init(PACKAGE_NAME, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	g_set_prgname(PACKAGE_NAME);

	// --
	// Parse commandline
	// --

	// Create option context
	opt_ctx = g_option_context_new("Eina options");
	g_option_context_set_help_enabled(opt_ctx, TRUE);
	g_option_context_set_ignore_unknown_options(opt_ctx, TRUE); // Ignore unknow
	g_option_context_add_main_entries(opt_ctx, opt_entries, GETTEXT_PACKAGE);

	// Options from underlying libraries
	g_option_context_add_group(opt_ctx, lomo_get_option_group());
	g_option_context_add_group(opt_ctx, gtk_get_option_group (TRUE));

	if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
	{
		gel_error("Option parsing failed: %s", err->message);
		g_error_free(err);
		exit (1);
	}
	g_option_context_free(opt_ctx);

	gtk_about_dialog_set_url_hook((GtkAboutDialogActivateLinkFunc) open_uri_hook, NULL, NULL);

	// --
	// Unique App stuff
	// --
#if HAVE_UNIQUE
	unique = unique_app_new_with_commands ("net.sourceforge.Eina", NULL,
		"play",    COMMAND_PLAY,
		"enqueue", COMMAND_ENQUEUE,
		NULL);
	if (unique_app_is_running(unique) && !opt_new_instance)
	{
		UniqueResponse response; /* the response to our command */
		UniqueMessageData *message;

		if (opt_uris != NULL)
		{
			message = unique_message_data_new ();
			unique_message_data_set_uris(message, opt_uris);
			response = unique_app_send_message(unique,
				opt_enqueue ? COMMAND_ENQUEUE : COMMAND_PLAY,
				message);
			unique_message_data_free(message);
		}

		g_object_unref(unique);
		return 0;
	}
#endif

	// --
	// Setup
	// --
	gel_set_debug_level(opt_debug_level);

	// Allow to read all files
	g_setenv("G_BROKEN_FILENAMES", "1", TRUE);

	// Checks
	#ifdef DIAGNOSE_GDK_PIXBUF
	inspect_gdk_pixbuf_loaders();
	#endif

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	eina_stock_init();

	// --
	// Create App and load modules
	// --
	app = gel_app_new();
	for (i = 0; modules[i]; i++)
	{
		GError *error = NULL;
		GelPlugin *plugin = gel_app_load_plugin_by_name(app, modules[i],& error);
		if (plugin == NULL)
		{
			gel_error("Cannot load %s: %s", modules[i], error->message);
			g_error_free(error);
		}
		else
			gel_plugin_add_lock(plugin);
	}

	// --
	// Set some signals
	// --
	gel_app_set_dispose_callback(app, app_dispose_cb, (gpointer) modules);

#if HAVE_UNIQUE
	unique_app_watch_window(unique, (GtkWindow *) GEL_APP_GET_WINDOW(app));
	g_signal_connect (unique, "message-received", G_CALLBACK (unique_message_received_cb), app);
#endif

	// Add files from cmdl
	if (opt_uris)
	{
		gint i;
		gchar *uri;
		GSList *uris = NULL;

		for (i = 0; (opt_uris != NULL) && (opt_uris[i] != NULL); i++)
			if ((uri = lomo_create_uri(opt_uris[i])) != NULL)
			{
				uris = g_slist_prepend(uris, g_file_new_for_uri(uri));
				g_free(uri);
			}
		uris = g_slist_reverse(uris);
		g_strfreev(opt_uris);

		gel_io_list_read(uris, "standard::*",
			list_read_success_cb, list_read_error_cb,
			(gpointer) app);
	}

	// Load pl
	else
	{
		gchar *buff = NULL;
		GError *err = NULL;
		gchar *file = gel_app_build_config_filename("playlist", FALSE, -1, &err);
		if (file == NULL)
		{
			gel_warn("Cannot load playlist: %s", err->message);
			g_error_free(err);
		}
		else
		{
			if (g_file_get_contents(file, &buff, NULL, NULL))
			{
				gchar **uris = g_uri_list_extract_uris((const gchar *) buff);
				lomo_player_append_uri_strv(GEL_APP_GET_LOMO(app), uris);
				gint current = eina_conf_get_int(GEL_APP_GET_SETTINGS(app), "/playlist/last_current", 0);
				if (current >= 0)
					lomo_player_go_nth( GEL_APP_GET_LOMO(app), current, NULL);
				g_strfreev(uris);
				g_free(buff);
			}
			g_free(file);
		}
	}

	gtk_main();

#if HAVE_UNIQUE
	g_object_unref(unique);
#endif
	return 0;
}

#if HAVE_UNIQUE
// --
// UniqueApp stuff 
// --
static UniqueResponse
unique_message_received_cb (UniqueApp *unique,
	UniqueCommand      command,
	UniqueMessageData *message,
	guint              time_,
	gpointer           data)
{
	UniqueResponse res = UNIQUE_RESPONSE_PASSTHROUGH;
	GelApp *app = GEL_APP(data);
	gchar **uris;

	// --
	// Handle play or enqueue with usable uris
	// --
	if ((((command == COMMAND_PLAY) || command == COMMAND_ENQUEUE)) &&
		(uris = unique_message_data_get_uris(message)))
	{
		GSList *gfiles = NULL;
		gint i;
		gchar *uri;

		for (i = 0; (uris != NULL) && (uris[i] != NULL); i++)
			if ((uri = lomo_create_uri(uris[i])) != NULL)
			{
				gfiles = g_slist_prepend(gfiles, g_file_new_for_uri(uri));
				g_free(uri);
			}
		g_strfreev(uris);

		gfiles = g_slist_reverse(gfiles);

		if (command == COMMAND_PLAY)
			lomo_player_clear(GEL_APP_GET_LOMO(app));
		gel_io_list_read(gfiles, "standard::*",
			list_read_success_cb, list_read_error_cb,
			app);
		gtk_main();

		res = UNIQUE_RESPONSE_OK;
	}

	return res;
}
#endif

// --
// Callbacks
// --
static void
list_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data)
{
	GelApp *app = GEL_APP(data);
	GList *results = gel_io_op_result_get_object_list(res);
	GSList *filter = gel_list_filter(results, (GelFilterFunc) eina_fs_is_supported_file, NULL);
	GList *uris = NULL;
	while (filter)
	{
		uris = g_list_prepend(uris, g_file_get_uri(G_FILE(filter->data)));
		filter = filter->next;
	}
	gel_list_deep_free(results, g_object_unref);

	lomo_player_append_uri_multi(GEL_APP_GET_LOMO(app), uris);
	gel_slist_deep_free(gel_io_list_read_get_sources(op), g_object_unref);
	gel_list_deep_free(uris, g_free);

	if (gtk_main_level() > 1)
		gtk_main_quit();
}

static void
list_read_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data)
{
	gchar *uri = g_file_get_uri(source);
	gel_error("Error while getting info for '%s': %s", uri, error->message);
	g_free(uri);
}

static void
app_dispose_cb(GelApp *app, gpointer data)
{
	gchar **modules = (gchar **) data;
	gint i = 0;
	while (modules[i]) i++; i--; // Count how many modules

	for (;i >= 0; i--)
	{
		GError *error = NULL;
		GelPlugin *plugin = gel_app_get_plugin_by_name(app, modules[i]);
		if (plugin == NULL)
		{
			gel_error(N_("Cannot find loaded plugin %s"), modules[i]);
			continue;
		}
	
		gel_plugin_remove_lock(plugin);
		if (!gel_app_unload_plugin(app, plugin, &error))
		{
			gel_error(N_("Cannot fini plugin %s: %s"), gel_plugin_stringify(plugin), error->message);
			g_error_free(error);
			continue;
		}
	}
	gel_app_purge(app);

	gtk_main_quit();
}

// --
// XXX ugly hack
// --
void
xxx_ugly_hack(void)
{
	curl_engine_new();
	gel_list_join(" ", NULL);
	gel_io_file_get_child_for_file_info(NULL, NULL);
	gel_io_recurse_tree_new();
}

