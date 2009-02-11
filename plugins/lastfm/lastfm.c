/*
 * plugins/lastfm/lastfm.c
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

#include <config.h>
#include "lastfm.h"
#include "submit.h"
#include "artwork.h"

struct _LastFM {
	LastFMArtwork *artwork;
};

// --
// Main plugin
// --
gboolean
lastfm_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFM *self = g_new0(LastFM, 1);
	plugin->data = self;

	return lastfm_artwork_init(app, plugin, error);
#if 0
	plugin->data = g_new0(LastFM, 1);

	gchar *ui_path   = NULL;
	gchar *icon_path = NULL;

	GtkBuilder *builder = NULL;
	GtkWidget  *icon = NULL, *widget = NULL;

	// Submit handling
	if (!lastfm_submit_init(plugin, error))
		goto lastfm_init_fail;

	ui_path = eina_plugin_build_resource_path(plugin, "lastfm.ui");
	builder = gtk_builder_new();

	if (gtk_builder_add_from_file(builder, ui_path, NULL) == 0)
		goto lastfm_init_fail;
	
	if ((widget = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"))) == NULL)
		goto lastfm_init_fail;
	if ((widget = gtk_bin_get_child(GTK_BIN(widget))) == NULL)
		goto lastfm_init_fail;
	g_object_ref(widget);
	gtk_widget_destroy(widget->parent);
	gtk_widget_show_all(widget);

	icon_path = eina_plugin_build_resource_path(plugin, "lastfm.png");
	if ((icon = gtk_image_new_from_file(icon_path)) == NULL)
		goto lastfm_init_fail;

	GtkWidget *parent = gtk_widget_get_parent(widget);
	g_object_ref(widget);
	gtk_container_remove(GTK_CONTAINER(parent), widget);
	gtk_widget_destroy(parent);
	eina_plugin_add_configuration_widget(plugin, GTK_IMAGE(icon), (GtkLabel*) gtk_label_new(N_("LastFM")), widget);
	g_object_unref(widget);

	EINA_PLUGIN_DATA(plugin)->configuration_widget = widget;

	return TRUE;

lastfm_init_fail:
	gel_free_and_invalidate(ui_path, NULL, g_free);
	gel_free_and_invalidate(builder, NULL, g_object_unref);

	gel_free_and_invalidate(icon_path, NULL, g_free);
	gel_free_and_invalidate(icon,      NULL, g_object_unref);

	gel_free_and_invalidate(widget,    NULL, g_object_unref);

	if (EINA_PLUGIN_DATA(plugin)->submit != NULL)
	{
		lastfm_submit_exit(plugin, NULL);
		EINA_PLUGIN_DATA(plugin)->submit = NULL;
	}

	gel_free_and_invalidate(plugin->data, NULL, g_free);
	
	return FALSE;
#endif
}

gboolean
lastfm_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
#if 0
	eina_plugin_remove_configuration_widget(plugin, EINA_PLUGIN_DATA(plugin)->configuration_widget);
	if (!lastfm_submit_exit(plugin, error))
		return FALSE;

	g_free(plugin->data);
#endif
	if (!lastfm_artwork_fini(app, plugin, error))
		return FALSE;
	g_free(plugin->data);
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin lastfm_plugin = {
	EINA_PLUGIN_SERIAL, "lastfm", PACKAGE_VERSION,
	N_("Lastfm integration"),
	N_("Lastfm integration:\n"
	"· Submit played streams to last.fm"),
	"lastfm.png", EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	lastfm_init, lastfm_fini,

	NULL, NULL
};
