#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gel/gel.h"

#include <eina/ext/eina-stock.h>

gint main(gint argc, gchar *argv[])
{
	g_type_init();
	gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	gtk_init(&argc, &argv);

	// Initialize stock icons stuff
	gchar *themedir = g_build_filename(PACKAGE_DATA_DIR, "icons", NULL);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	g_free(themedir);

	if ((themedir = (gchar*) g_getenv("EINA_THEME_DIR")) != NULL)
		gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), themedir);
	eina_stock_init();

	GelPluginEngine *engine = gel_plugin_engine_new(&argc, &argv);

	gchar *plugins[] = { "player", "playlist" };
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
		GtkApplication *app = GTK_APPLICATION(gel_plugin_engine_get_interface(engine, "application"));
		gtk_application_run(app);
	}

	g_object_unref(engine);
	return 0;
}


