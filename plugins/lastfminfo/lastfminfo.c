/*
 * plugins/lastfminfo/lastfminfo.c
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

#define GEL_DOMAIN "Eina::Plugin::LastFM-Info"
#define PLUGIN_NAME "lastfminfo"
#define EINA_PLUGIN_DATA_TYPE LastFMInfoData 

#include <string.h>
#include <WebKit/webkitwebview.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>
#include <eina/iface.h>

typedef struct LastFMInfoData {
	GtkWidget *dock_widget;
	GtkWidget *loading_widget;
	WebKitWebView *webview_widget;
	gboolean is_updated;
} LastFMInfoData;

/*
 * Widget definitions 
 */
GtkWidget*
lastfm_info_dock_new(EinaPlugin *plugin);

// Xfer callbacks
void
on_lastfm_info_xfer_error(GelIOAsyncReadOp *xfer, GError *error, gpointer data);

void
on_lastfm_info_xfer_finish(GelIOAsyncReadOp *xfer, GByteArray *xfer_data, gpointer data);

// Lomo Callbacks
void
on_lastfm_info_lomo_change_change(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self);

void
on_lastfm_info_xfer_error(GelIOAsyncReadOp *xfer, GError *error, gpointer data)
{
	gel_error("Cannot get buffer: '%s'", error->message);
	g_idle_add((GSourceFunc) g_object_unref, xfer);
}

void
on_lastfm_info_xfer_finish(GelIOAsyncReadOp *xfer, GByteArray *xfer_data, gpointer data)
{
	EinaPlugin *plugin = (EinaPlugin *) data;
	LastFMInfoData *self = EINA_PLUGIN_GET_DATA(plugin);

	gchar *token1 = "<div id=\"wiki\">";
	gchar *token2 =  "</div><!-- #wiki -->";

	gchar *tmp, *tmp2;
	gchar *webview_buffer = NULL;
	
	gel_info("Data fetched. %ld bytes", xfer_data->len);
	g_file_set_contents("/tmp/lala2", (gchar *) xfer_data->data, xfer_data->len, NULL);

	if ((tmp = g_strstr_len((gchar*) xfer_data->data, xfer_data->len, token1)) == NULL)
	{
		gel_error("Cannot find toke1");
		return;
	}
	tmp = tmp + (strlen(token1)*sizeof(gchar));
	webview_buffer = g_strdup(tmp);

	if ((tmp2 = g_strstr_len(webview_buffer, strlen(webview_buffer), token2)) == NULL)
	{
		gel_error("Cannot find token2");
		g_free(tmp);
		return;
	}
	tmp2[0] = '\0';

	webkit_web_view_load_html_string(self->webview_widget, (const gchar*) webview_buffer, NULL);
	g_free(webview_buffer);

	// g_idle_add((GSourceFunc) g_object_unref, xfer);
}

void
on_lastfm_info_lomo_change_change(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self)
{
	GelIOAsyncReadOp *xfer = gel_io_async_read_op_new("http://www.last.fm/music/The+Cardigans/+wiki?setlang=en");
	g_signal_connect(xfer, "error",  G_CALLBACK(on_lastfm_info_xfer_error),  self);
	g_signal_connect(xfer, "finish", G_CALLBACK(on_lastfm_info_xfer_finish), self);

	eina_iface_info("Change from %d to %d", from, to);
}

GtkWidget *
lastfm_info_dock_new(EinaPlugin *plugin)
{
	LastFMInfoData *self = EINA_PLUGIN_GET_DATA(plugin);

	self->dock_widget =  gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->dock_widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	self->webview_widget = WEBKIT_WEB_VIEW(webkit_web_view_new());
	webkit_web_view_load_html_string(self->webview_widget, "<h1>Hello</h1>", NULL);

	gtk_container_add(GTK_CONTAINER(self->dock_widget), GTK_WIDGET(self->webview_widget));

	return GTK_WIDGET(self->dock_widget);
}

EINA_PLUGIN_FUNC gboolean
lastfm_info_exit(EinaPlugin *self)
{
	eina_plugin_free(self);
	return TRUE;
}

EINA_PLUGIN_FUNC EinaPlugin*
lastfminfo_init(GelHub *app, EinaIFace *iface)
{
	gchar *icon_pathname;

	// Create struct
	EinaPlugin *self = eina_plugin_new(iface,
		PLUGIN_NAME, "lastfminfo", g_new0(EINA_PLUGIN_DATA_TYPE, 1), lastfm_info_exit,
		NULL, NULL, NULL);

	// Create dock widgets
	self->dock_widget = lastfm_info_dock_new(self);
	gtk_widget_show_all(GTK_WIDGET(self->dock_widget));

	icon_pathname = eina_iface_plugin_resource_get_pathname(self, "lastfm-icon.png");
	if (icon_pathname == NULL)
		gel_error("Cannot load lastfm-icon.png");
	else
	{
		self->dock_label_widget = gtk_image_new_from_file(icon_pathname);
		g_free(icon_pathname);
		gtk_widget_show_all(GTK_WIDGET(self->dock_label_widget));
	}

	// Setup signals
	self->change = on_lastfm_info_lomo_change_change;

	return self;
}

