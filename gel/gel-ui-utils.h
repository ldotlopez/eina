/*
 * gel/gel-ui-utils.h
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

#ifndef _GEL_UI_UTILS_H
#define _GEL_UI_UTILS_H

#include <gel/gel-ui.h>

G_BEGIN_DECLS

// #define GelUI GtkBuilder

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

enum {
	GEL_UI_ERROR_NO_ERROR = 0,
    GEL_UI_ERROR_RESOURCE_NOT_FOUND 
};

/*
 * UI creation
 */
GtkBuilder *
gel_ui_load_resource(gchar *ui_filename, GError **error);

/*
 * Signal handling
 */
gboolean
gel_ui_signal_connect_from_def(GtkBuilder *ui, GelUISignalDef def, gpointer data, GError **error);

gboolean
gel_ui_signal_connect_from_def_multiple(GtkBuilder *ui, GelUISignalDef defs[], gpointer data, guint *count);

/*
 * Images on UI's
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error);

gboolean
gel_ui_load_image_from_def(GtkBuilder *ui, GelUIImageDef *def, GError **error);

gboolean
gel_ui_load_image_from_def_multiple(GtkBuilder *ui, GelUIImageDef defs[], guint *count);

/*
 * Widget utils
 */
void
gel_ui_container_replace_children(GtkContainer *container, GtkWidget *widget);

void
gel_ui_container_clear(GtkContainer *container);

GtkWidget *
gel_ui_container_find_widget(GtkContainer *container, gchar *name);

/*
 * Gtk List Model helpers
 */
gint *
gel_ui_tree_view_get_selected_indices(GtkTreeView *tv);

gboolean
gel_ui_list_model_get_iter_from_index(GtkListStore *model, GtkTreeIter *iter, gint index);

void
gel_ui_list_store_insert_at_index(GtkListStore *model, gint index, ...);

void
gel_ui_list_store_set_valist_at_index(GtkListStore *model, gint index, ...);

void
gel_ui_list_store_remove_at_index(GtkListStore *model, gint index);

/*
 * DnD
 */
void
gel_ui_widget_enable_drop(GtkWidget *widget, GCallback callback, gpointer user_data);

void
gel_ui_widget_disable_drop(GtkWidget *widget);

G_END_DECLS
#endif // _GEL_UI_H
