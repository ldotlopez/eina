#define GEL_DOMAIN "Eina::Main"
#include "config.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gel/gel-io.h>
#include "iface.h"

static gint opt_debug_level = GEL_DEBUG_LEVEL_WARN;
static const GOptionEntry opt_entries[] =
{
	{ "debug-level", 'd', 0, G_OPTION_ARG_INT, &opt_debug_level, "Debug level", "N" },
	{ NULL }
};

void xxx_ugly_hack(void)
{
	gel_glist_join(" ", NULL);
	gel_io_async_read_op_new(NULL);
}

void on_app_dispose(GelHub *app, gpointer data)
{
	gchar **modules = (gchar **) data;
	gint i = -1;

	while(modules[++i]);
	while(i) gel_hub_unload(app, modules[--i]);

	exit(0);
}

gint main
(gint argc, gchar *argv[])
{
	GelHub         *app;
	gint            i = 0;
	gchar          *modules[] = { "lomo", "log", "player", "iface", "playlist", "vogon", NULL};
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

	opt_ctx = g_option_context_new("Eina options");
	g_option_context_set_help_enabled(opt_ctx, TRUE);
	g_option_context_set_ignore_unknown_options(opt_ctx, TRUE);
	g_option_context_add_main_entries(opt_ctx, opt_entries, GETTEXT_PACKAGE);

	g_option_context_add_group(opt_ctx, lomo_get_option_group());
	g_option_context_add_group(opt_ctx, gtk_get_option_group (TRUE));

	if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
	{
		gel_error("option parsing failed: %s", err->message);
		g_error_free(err);
		exit (1);
	}
	g_option_context_free(opt_ctx);

	gel_set_debug_level(opt_debug_level);
	g_setenv("G_BROKEN_FILENAMES", "1", TRUE);
	tmp = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, NULL);
	g_mkdir(tmp, 00700);
	g_free(tmp);

	app = gel_hub_new(&argc, &argv);
	gel_hub_set_dispose_callback(app, on_app_dispose, modules);

	i = 0;
	while (modules[i])
		gel_hub_load(app, modules[i++]);

	eina_iface_load_plugin(gel_hub_shared_get(app, "iface"), "recently");
	eina_iface_load_plugin(gel_hub_shared_get(app, "iface"), "cover2");
	// eina_iface_load_plugin(gel_hub_shared_get(app, "iface"), "lastfminfo");

	gtk_main();
	return 0;
}
