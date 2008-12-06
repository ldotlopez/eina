#include <gtk/gtk.h>
#include "gel-ui-background-box.h"

GtkWindow *win;
GtkLabel *label;
GelUIBackgroundBox *ev;

gboolean do_it(gpointer d)
{
	GtkWindow *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize(w, 200, 200);
	GdkPixmap *snap = gtk_widget_get_snapshot(win, NULL);
	GelUIBackgroundBox *e = gel_ui_background_box_new();
	gtk_container_add(w, e);
	gel_ui_background_set_drawable(e, GDK_DRAWABLE(snap));
	gtk_widget_show_all(w);

	return FALSE;
}

gint main()
{
	gtk_init(NULL, NULL);

	label = (GtkLabel *) gtk_label_new("Test");
	win = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	ev = gel_ui_background_box_new();

	gtk_window_resize(win, 200, 200);
	gtk_container_add(GTK_CONTAINER(ev), GTK_WIDGET(label));
	gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(ev));
	gel_ui_background_set_pixbuf(ev, gdk_pixbuf_new_from_file ("logo.png", NULL));
	gtk_widget_show_all(GTK_WIDGET(win));

	g_signal_connect(win, "delete-event", gtk_main_quit, NULL);
	g_timeout_add(2000, do_it, NULL);
	gtk_main();

	return 0;
}
