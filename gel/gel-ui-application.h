/*
 * gel/gel-ui-application.h
 *
 * Copyright (C) 2004-2010 GelUI
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GEL_UI_APPLICATION_H__
#define __GEL_UI_APPLICATION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GEL_UI_TYPE_APPLICATION gel_ui_application_get_type()

#define GEL_UI_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_UI_TYPE_APPLICATION, GelUIApplication))

#define GEL_UI_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GEL_UI_TYPE_APPLICATION, GelUIApplicationClass))

#define GEL_UI_IS_APPLICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_UI_TYPE_APPLICATION))

#define GEL_UI_IS_APPLICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_UI_TYPE_APPLICATION))

#define GEL_UI_APPLICATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_UI_TYPE_APPLICATION, GelUIApplicationClass))

typedef struct {
	GtkApplication parent;
} GelUIApplication;

typedef struct {
	GtkApplicationClass parent_class;
} GelUIApplicationClass;

GType gel_ui_application_get_type (void);

GelUIApplication* gel_ui_application_new (gchar *application_id, gint *argc, gchar ***argv);

GtkWindow*       gel_ui_application_get_window             (GelUIApplication *self);
GtkUIManager*    gel_ui_application_get_window_ui_manager  (GelUIApplication *self);
GtkActionGroup*  gel_ui_application_get_window_action_group(GelUIApplication *self);
GtkVBox*         gel_ui_application_get_window_content_area(GelUIApplication *self);

GSettings*       gel_ui_application_get_settings           (GelUIApplication *self, gchar *subdomain);

gboolean         gel_ui_application_set_shared(GelUIApplication *self, gchar *key, gpointer data);
gpointer         gel_ui_application_get_shared(GelUIApplication *self, gchar *key);

G_END_DECLS

#endif 

