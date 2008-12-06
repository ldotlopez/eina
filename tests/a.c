#include <gtk/gtk.h>
#include "brasero-project-type-chooser.h"

GtkWindow *win;
GtkLabel *label, *label2;
GtkWindow *win2;

gboolean do_snap()
{
	GdkPixmap *snapshot = gtk_widget_get_snapshot(GTK_WIDGET(win), NULL);
	win2 = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	label2 = (GtkLabel *) gtk_label_new("                      \n                 \n               ");
	gtk_container_add(GTK_CONTAINER(win2), GTK_WIDGET(label2));
	gtk_widget_show_all(GTK_WIDGET(win2));
	/*
	gint i;
	GtkStyle *s = gtk_style_new();
	for (i = 0; i < 5; i++)
		s->bg_pixmap[i] = snapshot;
	s = gtk_style_attach(s, win2);
	*/
	// gdk_window_set_back_pixmap(GTK_WIDGET(win2)->window, snapshot, FALSE);
	// gdk_window_set_back_pixmap(GTK_WIDGET(label2)->window, snapshot, FALSE);
	/*
	gdk_draw_drawable(GDK_DRAWABLE(GTK_WIDGET(win2)->window), gdk_gc_new(GTK_WIDGET(win2)->window),
		GDK_DRAWABLE(GTK_WIDGET(win)->window), 0,0,0,0, -1, -1);
	gdk_draw_drawable(GDK_DRAWABLE(GTK_WIDGET(label2)->window), gdk_gc_new(GTK_WIDGET(win2)->window),
		GDK_DRAWABLE(GTK_WIDGET(win)->window), 0,0,0,0, -1, -1);
	*/
	GdkPixbuf *pb = gdk_pixbuf_new_from_file("/home/xuzo/svn/eina/trunk/pixmaps/logo.png", NULL);
	gdk_draw_pixbuf(GTK_WIDGET(win2)->window, GTK_WIDGET(win2)->style->white_gc, pb,
		0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NORMAL, 0, 0);
	
	return FALSE;
}

gint main()
{
	gtk_init(NULL, NULL);

	label = (GtkLabel *) gtk_label_new("Test");
	win = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	BraseroProjectTypeChooser *ev = brasero_project_type_chooser_new(label);
	gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(ev));
	gtk_widget_show_all(GTK_WIDGET(win));

	g_signal_connect(win, "delete-event", gtk_main_quit, NULL);
	g_timeout_add(1000, do_snap, NULL);
	gtk_main();

	return 0;
}
