/*
 * eina/dock/eina-dock.h
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

#ifndef _EINA_DOCK
#define _EINA_DOCK

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_DOCK eina_dock_get_type()

#define EINA_DOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_DOCK, EinaDock))

#define EINA_DOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_DOCK, EinaDockClass))

#define EINA_IS_DOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_DOCK))

#define EINA_IS_DOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_DOCK))

#define EINA_DOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_DOCK, EinaDockClass))

typedef struct {
	GtkExpander parent;
} EinaDock;

typedef struct {
	GtkExpanderClass parent_class;
} EinaDockClass;

GType eina_dock_get_type (void);

EinaDock  *eina_dock_new (void);

gchar  **eina_dock_get_page_order(EinaDock *self);
void     eina_dock_set_page_order(EinaDock *self, gchar **order);

GtkWidget *eina_dock_get_widget(GtkWidget *owner);

gboolean eina_dock_add_widget   (EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *widget);
gboolean eina_dock_remove_widget(EinaDock *self, GtkWidget *widget);
gboolean eina_dock_remove_widget_by_id(EinaDock *self, gchar *id);
gboolean eina_dock_switch_widget(EinaDock *iface, gchar *id);

G_END_DECLS

#endif // _EINA_DOCK
