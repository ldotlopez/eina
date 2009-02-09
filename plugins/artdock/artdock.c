#define GEL_DOMAIN "Plugin::ArtDock"
#define VERSION "0.0.1"

#include <eina/eina-plugin.h>
#include <eina/art.h>

typedef struct {
	GelApp     *app;
	GelPlugin  *plugin;
	GtkImage   *dock;
	ArtBackend *backends[3];
} ArtDock;

// --
// mini plugins
// --
static void
mini_search_random_1(Art *art, ArtSearch *search, ArtDock *self)
{
	if (g_random_boolean())
	{
		gchar *path = gel_plugin_build_resource_path(self->plugin, "random-ok.png");
		art_report_success(art, search, gdk_pixbuf_new_from_file_at_size(path, 256, 256, NULL));
		g_free(path);
	}
	else
	{
		art_report_failure(art, search);
	}
}

static void
mini_search_random_2(Art *art, ArtSearch *search, ArtDock *self)
{
	if (g_random_boolean())
	{
		gchar *path = gel_plugin_build_resource_path(self->plugin, "100-ok.png");
		art_report_success(art, search, gdk_pixbuf_new_from_file_at_size(path, 256, 256, NULL));
		g_free(path);
	}
	else
	{
		art_report_failure(art, search);
	}
}

static void
mini_search_fail(Art *art, ArtSearch *search, ArtDock *self)
{
	art_report_failure(art, search);
}

static void
art_success_cb(Art *art, ArtSearch *search, ArtDock *self)
{
	gtk_image_set_from_pixbuf(self->dock, GDK_PIXBUF(art_search_get_result(search)));
}

static void
art_fail_cb(Art *art, ArtSearch *search, ArtDock *self)
{
	gchar *path = gel_plugin_build_resource_path(self->plugin, "fail.png");
	gtk_image_set_from_pixbuf(self->dock, gdk_pixbuf_new_from_file_at_size(path, 256, 256, NULL));
	g_free(path);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint old, gint new, ArtDock *self)
{
	Art *art = gel_app_shared_get(self->app, "art");
	if (art == NULL)
	{
		gel_error(N_("Cannot fetch Art object"));
		return;
	}

	art_search(art, lomo_player_get_nth(lomo, new),
		(ArtFunc) art_success_cb, (ArtFunc) art_fail_cb,
		self);
}

static gboolean
plugin_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	if (!gel_app_load_plugin_by_name(app, "art", error) || !gel_app_load_plugin_by_name(app, "lomo", error))
		return FALSE;

	ArtDock *self = g_new0(ArtDock, 1);
	self->dock   = (GtkImage *) gtk_image_new();
	self->app    = app;
	self->plugin = plugin;

	gtk_widget_set_size_request(GTK_WIDGET(self->dock), 256, 256);
	gtk_widget_show_all(GTK_WIDGET(self->dock));
	eina_plugin_add_dock_widget(plugin, "artdock", gtk_image_new_from_stock(GTK_STOCK_INFO,  GTK_ICON_SIZE_MENU), (GtkWidget *) self->dock);
	plugin->data = self;

	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	if (lomo == NULL)
		return FALSE;
	g_signal_connect(lomo, "change", (GCallback) lomo_change_cb, self);

	Art *art = GEL_APP_GET_ART(app);
	self->backends[0] = art_add_backend(art, (ArtFunc) mini_search_random_1, NULL, self);
	self->backends[1] = art_add_backend(art, (ArtFunc) mini_search_random_2, NULL, self);
	self->backends[2] = art_add_backend(art, (ArtFunc) mini_search_fail, NULL, self);

	return TRUE;
}

static gboolean
plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	ArtDock *self = (ArtDock*) plugin->data;
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	eina_plugin_remove_dock_widget(plugin, "artdock");

	if (lomo != NULL)
		g_signal_handlers_disconnect_by_func(lomo, lomo_change_cb, plugin->data);

	Art *art = GEL_APP_GET_ART(app);
	art_remove_backend(art, self->backends[0]);
	art_remove_backend(art, self->backends[1]);
	art_remove_backend(art, self->backends[2]);

	gel_app_unload_plugin_by_name(app, "art", NULL);
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin artdock_plugin = {
	EINA_PLUGIN_SERIAL,
	"artdock", VERSION,
	N_("ArtDock"), N_("Art framework test"), NULL,
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	plugin_init, plugin_fini,
	NULL, NULL
};

