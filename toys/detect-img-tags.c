#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <lomo/lomo-player.h>
#include <gtk/gtk.h>

LomoPlayer *lomo = NULL;

void tag_cb (LomoPlayer *player, LomoStream *stream, const gchar *tag)
{
	if  (!g_str_equal (tag, "image"))
		return;

	const GValue *v = lomo_stream_get_tag (stream, tag);
	GstBuffer *buffer  = GST_BUFFER(gst_value_get_buffer (v));
	GstCaps *caps = GST_BUFFER_CAPS(buffer);
	// g_debug ("Got image (%p) %d. %s",b->data, b->size, G_VALUE_TYPE_NAME(v));


	guint n_structs = gst_caps_get_size(caps);
	for (guint i = 0; i < n_structs; i++)
	{
		GstStructure *s = gst_caps_get_structure(caps, i);
		g_debug("Struct #%d: %s", i, gst_structure_get_name(s));

		if (!g_str_has_prefix(gst_structure_get_name(s), "image/"))
			continue;

		GInputStream *stream = g_memory_input_stream_new_from_data (buffer->data, buffer->size, NULL);

		GError *e = NULL;
		GdkPixbuf *pb = gdk_pixbuf_new_from_stream(stream, NULL, &e);
		if (!pb)
			g_debug("%s", e->message);
		else
		{
			GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_container_add((GtkContainer *) window, gtk_image_new_from_pixbuf(pb));
			gtk_widget_show_all(window);
		}
	}
}

gboolean idle(gpointer data)
{
	lomo_player_insert_uri(lomo, "/mnt/tank/Music/Amaral/Amaral - Amaral/Amaral - Amaral - [01] Rosita.mp3", 0);
	return FALSE;
}

gint main (gint argc, gchar *argv[])
{
	gtk_init(&argc, &argv);
	lomo_init(NULL, NULL);

	lomo = lomo_player_new("audio-output", "fakesink" ,NULL);

	g_signal_connect(lomo, "tag", (GCallback) tag_cb, NULL);
	// g_signal_connect_swapped(lomo, "all-tags", (GCallback) g_main_loop_quit, loop);

	g_idle_add(idle, NULL);
	gtk_main();

	return 0;
}

