#define GEL_DOMAIN "Eina::Main"
#include "config.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <unique/unique.h>
#include <gel/gel-io.h>
#include <lomo/player.h>
#include <lomo/util.h>
#include <eina/lomo.h>
#include <eina/player.h>
#include <eina/loader.h>

static gint     opt_debug_level = GEL_DEBUG_LEVEL_WARN;
static gboolean opt_enqueue = FALSE;
static gchar**  opt_uris = NULL;
static const GOptionEntry opt_entries[] =
{
	{ "debug-level", 'd', 0, G_OPTION_ARG_INT,  &opt_debug_level, "Debug level", "<int 0..5>" },
	{ "enqueue",     'e', 0, G_OPTION_ARG_NONE, &opt_enqueue,     "Enqueue files instead of directly play", NULL},

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opt_uris, NULL, "[FILE...]"},
	{ NULL }
};

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

// --
// Callbacks
// --
static void
on_app_dispose(GelHub *app, gpointer data);

// --
// XXX ugly hack
// --
void xxx_ugly_hack(void);

gint main
(gint argc, gchar *argv[])
{
	GelHub         *app;
	UniqueApp      *unique = NULL;
	gint            i = 0;
	gchar          *modules[] = { "lomo", "artwork", "player", "loader", "dock", "playlist", "plugins", "vogon", NULL};
	gchar          *tmp;

	GOptionContext *opt_ctx;
	GError *err = NULL;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_init(&argc, &argv);
	lomo_init(&argc, &argv);
	gel_init(PACKAGE_NAME, PACKAGE_DATA_DIR);

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

	// --
	// Unique App stuff
	// --

	unique = unique_app_new_with_commands ("net.sourceforge.Eina", NULL,
		"play",    COMMAND_PLAY,
		"enqueue", COMMAND_ENQUEUE,
		NULL);
	if (unique_app_is_running(unique))
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

	// --
	// Setup
	// --
	gel_set_debug_level(opt_debug_level);

	// Allow to read all files
	g_setenv("G_BROKEN_FILENAMES", "1", TRUE);
	tmp = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, NULL);
	g_mkdir(tmp, 00700);
	g_free(tmp);

	// --
	// Create App and load modules
	app = gel_hub_new(&argc, &argv);

	for (i = 0; modules[i]; i++)
		gel_hub_load(app, modules[i]);

/*
	// Add or enqueue files from cmdl
	LomoPlayer *lomo = GEL_HUB_GET_LOMO(app);
	GList *uris = NULL;

	if ((!opt_enqueue) && (opt_uris != NULL))
		lomo_player_clear(lomo);

	for (i = 0; (opt_uris != NULL) && (opt_uris[i] != NULL); i++)
	{
		if ((tmp = lomo_create_uri(opt_uris[i])) != NULL)
		{
			uris = g_list_prepend(uris, tmp);
		}
	}
	lomo_player_add_uri_multi(lomo, uris);
	gel_list_deep_free(uris, g_free);
	g_strfreev(opt_uris);
*/

	// --
	// Set some signals
	// --
	unique_app_watch_window(unique, eina_player_get_main_window(GEL_HUB_GET_PLAYER(app)));
	gel_hub_set_dispose_callback(app, on_app_dispose, modules);
	g_signal_connect (unique, "message-received", G_CALLBACK (unique_message_received_cb), app);

	gtk_main();

	g_object_unref(unique);
	return 0;
}

// --
// UniqueApp stuff 
// --
static void
unique_async_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data)
{
	GelHub *app = GEL_HUB(data);
	GList *results = gel_io_op_result_get_object_list(res);
	GSList *filter = gel_list_filter(results, (GelFilterFunc) eina_fs_is_supported_file, NULL);
	GList *uris = NULL;
	while (filter)
	{
		uris = g_list_prepend(uris, g_file_get_uri(G_FILE(filter->data)));
		filter = filter->next;
	}
	gel_list_deep_free(results, g_object_unref);

	lomo_player_add_uri_multi(GEL_HUB_GET_LOMO(app), uris);
	gel_list_deep_free(uris, g_free);

	if (gtk_main_level() > 1)
		gtk_main_quit();
}

static void
unique_async_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data)
{
	gel_error("Error while getting info for '%s': %s", error->message);
}

static UniqueResponse
unique_message_received_cb (UniqueApp *unique,
	UniqueCommand      command,
	UniqueMessageData *message,
	guint              time_,
	gpointer           data)
{
	UniqueResponse res = UNIQUE_RESPONSE_PASSTHROUGH;
	GelHub *app = GEL_HUB(data);
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
			lomo_player_clear(GEL_HUB_GET_LOMO(app));
		gel_io_list_read(gfiles, "standard::*",
			unique_async_success_cb, unique_async_error_cb,
			app);
		gtk_main();
		gel_slist_deep_free(gfiles, g_object_unref);

		res = UNIQUE_RESPONSE_OK;
	}

	return res;
}

// --
// Callbacks
// --
void on_app_dispose(GelHub *app, gpointer data)
{
	gchar **modules = (gchar **) data;
	gint i = -1;

	while(modules[++i]);
	while(i) gel_hub_unload(app, modules[--i]);

	exit(0);
}

// --
// XXX ugly hack
// --
void
xxx_ugly_hack(void)
{
	gel_list_join(" ", NULL);
	gel_io_file_get_child_for_file_info(NULL, NULL);
	gel_io_recurse_tree_new();
}

