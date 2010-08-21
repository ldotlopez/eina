#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gel/gel.h"
#include "eina/lomo/lomo.h"
#include "eina/ext/eina-stock.h"

static gboolean opt_enqueue = FALSE;
static gboolean opt_new_instance = FALSE;
static gchar**  opt_uris = NULL;
static const GOptionEntry opt_entries[] =
{
	{ "enqueue",      'e', 0, G_OPTION_ARG_NONE, &opt_enqueue,      "Enqueue files instead of directly play", NULL},
	{ "new-instance", 'n', 0, G_OPTION_ARG_NONE, &opt_new_instance, "Ignore running instances and create a new one", NULL},

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opt_uris, NULL, "[FILE...]"},
	{ NULL }
};

gint main(gint argc, gchar *argv[])
{
	g_type_init();
	gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	gtk_init(&argc, &argv);

	GOptionContext *opt_ctx = g_option_context_new("Eina options");
	g_option_context_set_help_enabled(opt_ctx, TRUE);
	g_option_context_set_ignore_unknown_options(opt_ctx, TRUE); // Ignore unknow
	g_option_context_add_main_entries(opt_ctx, opt_entries, GETTEXT_PACKAGE);

	// Options from underlying libraries
	g_option_context_add_group(opt_ctx, lomo_get_option_group());
	g_option_context_add_group(opt_ctx, gtk_get_option_group (TRUE));

	GError *err = NULL;
	if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
	{
		g_warning("Option parsing failed: %s", err->message);
		g_error_free(err);
		return 1;
	}
	g_option_context_free(opt_ctx);

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	eina_stock_init();

	GelUIApplication *application = NULL;
	GelPluginEngine *engine = gel_plugin_engine_new(&argc, &argv);

	if (!gel_plugin_engine_load_plugin_by_name(engine, "application", NULL) ||
		!(application = gel_plugin_engine_get_interface(engine, "application")))
	{
		g_warning(N_("Unable to initialize application interface, aborting"));
		return 1;
	}

	gchar *plugins[] =
	{
	"player", "playlist"
	};
	guint  n_plugins = G_N_ELEMENTS(plugins);
	guint  i;
	for (i = 0; i < n_plugins; i++)
	{
		GError *error = NULL;
		if (!gel_plugin_engine_load_plugin_by_name(engine, plugins[i], &error))
		{
			g_warning(N_("Unable to load required plugin '%s': %s"), plugins[i], error->message);
			g_error_free(error);
			break;
		}
	}

	if (i == n_plugins)
	{
		GList *l = NULL;
		for (guint u = 0; opt_uris && opt_uris[u]; u++)
		{
			gchar *uri = lomo_create_uri(opt_uris[u]);
			if (uri)
				l = g_list_prepend(l, uri);
		}
		l = g_list_reverse(l);
		eina_fs_load_from_uri_multiple(engine, l);
		gel_list_deep_free(l, (GFunc) g_free);
		gtk_application_run((GtkApplication *) application);
	}

	g_object_unref(engine);
	return 0;
}


