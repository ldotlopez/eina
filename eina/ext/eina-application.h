/*
 * eina/ext/eina-application.h
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _EINA_APPLICATION
#define _EINA_APPLICATION

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_APPLICATION eina_application_get_type()

#define EINA_APPLICATION(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_APPLICATION, EinaApplication))

#define EINA_APPLICATION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_APPLICATION, EinaApplicationClass))

#define EINA_IS_APPLICATION(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_APPLICATION))

#define EINA_IS_APPLICATION_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_APPLICATION))

#define EINA_APPLICATION_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_APPLICATION, EinaApplicationClass))

typedef struct {
	GtkApplication parent;
} EinaApplication;

typedef struct {
	GtkApplicationClass parent_class;
} EinaApplicationClass;

GType eina_application_get_type (void);

EinaApplication* eina_application_new (gint *argc, gchar ***argv);

GtkWindow*       eina_application_get_window             (EinaApplication *self);
GtkUIManager*    eina_application_get_window_ui_manager  (EinaApplication *self);
GtkActionGroup*  eina_application_get_window_action_group(EinaApplication *self);
GtkVBox*         eina_application_get_window_content_area(EinaApplication *self);

GSettings*       eina_application_get_settings           (EinaApplication *self, gchar *subdomain);

gboolean         eina_application_set_shared(EinaApplication *self, gchar *key, gpointer data);
gpointer         eina_application_get_shared(EinaApplication *self, gchar *key);

G_END_DECLS

#endif 

