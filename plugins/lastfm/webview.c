/*
 * plugins/lastfm/webview.c
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

#include "lastfm.h"
#include <webkit/webkit.h>

struct _LastFMWebView {
	GtkWidget *webview;
	GtkWidget *dock;
};

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self);

gboolean
lastfm_webview_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMWebView *self = g_new0(LastFMWebView, 1);

	self->webview = webkit_web_view_new();

	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	g_signal_connect(lomo, "change", (GCallback) lomo_change_cb, self);

	gchar *icon_path = gel_plugin_build_resource_path(plugin, "lastfm.png");

	GtkImage *img = NULL;
	if (icon_path)
	{
		img = (GtkImage *) gtk_image_new_from_file(icon_path);
		g_free(icon_path);
	}

	eina_plugin_add_dock_widget(plugin, "lastfm-webview", (GtkWidget *) img, self->webview);
	gtk_widget_show_all(self->webview);

	EINA_PLUGIN_DATA(plugin)->webview = self;

	return TRUE;
}

gboolean
lastfm_webview_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LastFMWebView *self = EINA_PLUGIN_DATA(plugin)->webview;

	eina_plugin_remove_dock_widget(plugin, "lastfm-webview");

	g_object_unref(self->webview);
	g_free(self);

	return TRUE;
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self)
{

}
