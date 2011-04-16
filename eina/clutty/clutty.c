/*
 * eina/clutty/clutty.c
 *
 * Copyright (C) 2004-2010 Eina
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

#include <eina/eina-plugin.h>
#include "eina-cover-clutter.h"
#include <eina/lomo/lomo.h>

#define STANDALONE_WINDOW 0

#if STANDALONE_WINDOW
GtkWindow *window = NULL;
EinaCoverClutter *clutty = NULL;
LomoStream *stream = NULL;
gulong stream_signal_id = 0;

static void
stream_metadata_updated_cb(LomoStream *stream, const gchar *key, gpointer data)
{
	if (!g_str_equal (key, "art-uri"))
		return;

	const gchar *uri = (const gchar *) lomo_stream_get_extended_metadata(stream, key);
	g_return_if_fail(uri);

	g_warning("Update ART with: %s", uri);
	GFile *f = g_file_new_for_uri(uri);
	GInputStream *s = G_INPUT_STREAM(g_file_read(f, NULL, NULL));
	g_object_unref(f);
	g_return_if_fail(G_IS_INPUT_STREAM(s));
	
	GdkPixbuf *pb = gdk_pixbuf_new_from_stream(s, NULL, NULL);
	g_object_unref(s);
	g_return_if_fail(GDK_IS_PIXBUF(pb));

	g_object_set(clutty, "cover", pb, NULL);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, gpointer data)
{
	g_return_if_fail(to >= 0);

	if (stream && stream_signal_id)
	{
		g_signal_handler_disconnect (G_OBJECT (stream), stream_signal_id);
	}

	stream = lomo_player_nth_stream(lomo, to);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	stream_metadata_updated_cb(stream, "art-uri", NULL);
	g_signal_connect(stream, "extended-metadata-updated", G_CALLBACK (stream_metadata_updated_cb), NULL);
}
#endif

G_MODULE_EXPORT gboolean
clutty_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	gtk_clutter_init(NULL, NULL);
	
	#if STANDALONE_WINDOW
	LomoPlayer *lomo = eina_application_get_lomo(app);
	g_signal_connect(lomo, "change", G_CALLBACK (lomo_change_cb), NULL);


	window = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize(window, 200, 200);

	clutty = eina_cover_clutter_new();
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (clutty));
	gtk_widget_show_all (GTK_WIDGET (window));
	#endif

	return TRUE;
}

G_MODULE_EXPORT gboolean
clutty_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	#if STANDALONE_WINDOW
	gtk_widget_destroy (GTK_WIDGET (window));
	#endif

	return TRUE;
}


