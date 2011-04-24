/*
 * tests/gel-background-box/test.c
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

#include <gtk/gtk.h>
#include "gel-ui-background-box.h"

GtkWindow *win;
GtkImage  *img;
GelUIBackgroundBox *ev;

gboolean do_it(gpointer d)
{
	GdkPixmap *snap = gtk_widget_get_snapshot(GTK_WIDGET(win), NULL);

	GtkWindow *w          = (GtkWindow*)          gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GelUIBackgroundBox *e = (GelUIBackgroundBox*) gel_ui_background_box_new();

	gel_ui_background_set_drawable(e, GDK_DRAWABLE(snap));

	gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(e));
	gtk_window_resize(w, 200, 200);
	gtk_widget_show_all(GTK_WIDGET(w));

	gint depth = gdk_drawable_get_depth(GDK_DRAWABLE(GTK_WIDGET(e)->window));

	GtkWindow *w2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(w2);

	GdkColor bg, fg;
	gdk_color_parse("#000000", &bg);
	gdk_color_parse("#000000", &fg);

	GdkPixmap *pm = // gdk_pixmap_new(NULL, 200, 200, depth);
		gdk_pixmap_new(GTK_WIDGET(w2)->window, 200, 200, -1);

	GdkGC * gc = gdk_gc_new(GDK_DRAWABLE(pm));
	gdk_gc_set_foreground(gc, &fg);
	gdk_draw_rectangle(GDK_DRAWABLE(pm),  gc,
		TRUE, 
		0, 0,
		100, 100);
		
	gtk_container_add(w2, gtk_image_new_from_pixmap(pm, NULL));
	gtk_widget_show_all(GTK_WIDGET(w2));

	return FALSE;
}

gint main()
{
	gtk_init(NULL, NULL);

	win = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	img = (GtkImage  *) gtk_image_new_from_file("logo.png");

	gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(img));
	gtk_widget_show_all(GTK_WIDGET(win));

	g_signal_connect(win, "delete-event", gtk_main_quit, NULL);
	g_timeout_add(2000, do_it, NULL);
	gtk_main();

	return 0;
}
