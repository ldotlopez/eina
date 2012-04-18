/*
 * eina/ext/eina-application.h
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

/* eina-application.h */

#ifndef _EINA_APPLICATION
#define _EINA_APPLICATION

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas.h>
#include <eina/ext/eina-window.h>

G_BEGIN_DECLS

#define EINA_TYPE_APPLICATION eina_application_get_type()
#define EINA_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_APPLICATION, EinaApplication))
#define EINA_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_APPLICATION, EinaApplicationClass))
#define EINA_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_APPLICATION))
#define EINA_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_APPLICATION))
#define EINA_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_APPLICATION, EinaApplicationClass))

typedef struct _EinaApplicationPrivate EinaApplicationPrivate;
typedef struct {
	/*< private >*/
	GtkApplication parent;
	EinaApplicationPrivate *priv;
} EinaApplication;

typedef struct {
	/*< private >*/
	GtkApplicationClass parent_class;
	gboolean (*action_activate) (EinaApplication *application, GtkAction *action);
} EinaApplicationClass;

GType eina_application_get_type (void);

EinaApplication* eina_application_new (const gchar *application_id);

gboolean eina_application_launch_for_uri(EinaApplication *application, const gchar *uri, GError **error);

gint*     eina_application_get_argc(EinaApplication *self);
gchar***  eina_application_get_argv(EinaApplication *self);

gboolean eina_application_set_interface  (EinaApplication *self, const gchar *name, gpointer interface);
gpointer eina_application_get_interface  (EinaApplication *self, const gchar *name);
gpointer eina_application_steal_interface(EinaApplication *self, const gchar *name);

EinaWindow*     eina_application_get_window             (EinaApplication *self);
GtkUIManager*   eina_application_get_window_ui_manager  (EinaApplication *self);
GtkActionGroup* eina_application_get_window_action_group(EinaApplication *self);

GSettings*      eina_application_get_settings(EinaApplication *self, const gchar *domain);

PeasEngine*     eina_application_create_standalone_engine(gboolean from_source);

G_END_DECLS

#endif /* _EINA_APPLICATION */
