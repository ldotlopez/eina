#include <glib.h>
#include <lomo/player.h>
#include <lomo/util.h>
#include "lastfm.h"

// --
// Main plugin
// --
gboolean
lastfm_init(EinaPlugin *plugin, GError **error)
{
	plugin->data = g_new0(LastFM, 1);

	// Submit handling
	if (!lastfm_submit_init(plugin, error))
		goto lastfm_init_fail;

	// Add preferences panel
	gchar *ui_path   = NULL;
	gchar *icon_path = NULL;

	GtkBuilder *builder = NULL;
	GtkWidget  *icon = NULL, *widget = NULL;

	ui_path = eina_plugin_build_resource_path(plugin, "lastfm.ui");
	builder = gtk_builder_new();

	if (gtk_builder_add_from_file(builder, ui_path, NULL) == 0)
		goto lastfm_init_fail;
	
	if ((widget = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"))) == NULL)
		goto lastfm_init_fail;
	if ((widget = gtk_bin_get_child(GTK_BIN(widget))) == NULL)
		goto lastfm_init_fail;

	icon_path = eina_plugin_build_resource_path(plugin, "lastfm.png");
	if ((icon = gtk_image_new_from_file(icon_path)) == NULL)
		goto lastfm_init_fail;

	eina_plugin_add_configuration_widget(plugin, GTK_IMAGE(icon), (GtkLabel*) gtk_label_new(N_("LastFM")), widget);
	EINA_PLUGIN_DATA(plugin)->configuration_widget = widget;

	return TRUE;

lastfm_init_fail:
	gel_free_and_invalidate(ui_path, NULL, g_free);
	gel_free_and_invalidate(builder, NULL, g_object_unref);

	gel_free_and_invalidate(icon_path, NULL, g_free);
	gel_free_and_invalidate(icon,      NULL, g_object_unref);

	gel_free_and_invalidate(widget,      NULL, g_object_unref);

	if (EINA_PLUGIN_DATA(plugin)->submit != NULL)
	{
		lastfm_submit_exit(plugin, NULL);
		EINA_PLUGIN_DATA(plugin)->submit = NULL;
	}

	gel_free_and_invalidate(plugin->data, NULL, g_free);
	
	return FALSE;
}

gboolean
lastfm_exit(EinaPlugin *plugin, GError **error)
{
	eina_plugin_remove_configuration_widget(plugin, EINA_PLUGIN_DATA(plugin)->configuration_widget);
	if (!lastfm_submit_exit(plugin, error))
		return FALSE;

	g_free(plugin->data);
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin lastfm_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Lastfm integration"),
	"0.0.1",
	N_("Lastfm integration"),
	N_("Lastfm integration:\n"
	"· Submit played streams to last.fm"),
	"lastfm.png",
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net/",

	lastfm_init, lastfm_exit,

	NULL, NULL
};
