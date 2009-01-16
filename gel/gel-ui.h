/*
 * gel/gel-ui.h
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

#ifndef _GEL_UI_H
#define _GEL_UI_H

#include <gtk/gtk.h>
#include <gel/gel.h>

G_BEGIN_DECLS

#define GelUI GtkBuilder

typedef struct GelUISignalDef
{
	gchar     *widget;
	gchar     *signal;
	GCallback  callback;
} GelUISignalDef;
#define GEL_UI_SIGNAL_DEF_NONE { NULL, NULL, NULL }

typedef struct GelUIImageDef
{
	gchar *widget;
	gchar *image;
	gint w, h;
}
GelUIImageDef;
#define GEL_UI_IMAGE_DEF_NONE { NULL, NULL, -1, -1 }

/*
 * UI creation
 */
GelUI *
gel_ui_load_resource(gchar *ui_filename, GError **error);

/*
 * Signal handling
 */
gboolean
gel_ui_signal_connect_from_def(GelUI *ui, GelUISignalDef def, gpointer data, GError **error);

gboolean
gel_ui_signal_connect_from_def_multiple(GelUI *ui, GelUISignalDef defs[], gpointer data, guint *count);

/*
 * Images on UI's
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error);

gboolean
gel_ui_load_image_from_def(GelUI *ui, GelUIImageDef *def, GError **error);

gboolean
gel_ui_load_image_from_def_multiple(GelUI *ui, GelUIImageDef defs[], guint *count);

/*
 * Stock icons
 */
gboolean
gel_ui_stock_add(gchar *resource, gchar *stock_name, gint size, GError **error);

void
gel_ui_container_replace_children(GtkContainer *container, GtkWidget *widget);

G_END_DECLS

#endif // _GEL_UI_H
