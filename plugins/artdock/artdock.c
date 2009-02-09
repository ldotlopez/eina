#define GEL_DOMAIN "Plugin::ArtDock"
#define VERSION "0.0.1"

#include <eina/eina-plugin.h>
#include <eina/art.h>

typedef struct {
	GelApp *app;
	GtkImage *dock;
} ArtDock;

static void
art_success_cb(Art *art, ArtSearch *search, ArtDock *self)
{
	gel_warn("Search success!!");
}

static void
art_fail_cb(Art *art, ArtSearch *search, ArtDock *self)
{
	gel_warn("Search fail!!");
}

static void
lomo_change_cb(LomoPlayer *lomo, gint old, gint new, ArtDock *self)
{
	g_warning("Got change %d => %d", old, new);
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
	self->dock = (GtkImage *) gtk_image_new();
	self->app  = app;
	
	eina_plugin_add_dock_widget(plugin, "artdock", gtk_image_new_from_stock(GTK_STOCK_INFO,  GTK_ICON_SIZE_MENU), (GtkWidget *) self->dock);
	plugin->data = self;

	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	if (lomo == NULL)
		return FALSE;
	
	g_signal_connect(lomo, "change", (GCallback) lomo_change_cb, self);
	return TRUE;
}

static gboolean
plugin_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	eina_plugin_remove_dock_widget(plugin, "artdock");

	if (lomo != NULL)
		g_signal_handlers_disconnect_by_func(lomo, lomo_change_cb, plugin->data);

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

