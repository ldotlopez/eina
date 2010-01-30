/*
 * eina/dock.h
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

#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_DOCK(p)           ((EinaDock *) p)
#define GEL_APP_GET_DOCK(app)  EINA_DOCK(gel_app_shared_get(app, "dock"))
#define EINA_OBJ_GET_DOCK(obj) GEL_APP_GET_DOCK(eina_obj_get_app(obj))

#define gel_app_get_dock(app)  EINA_DOCK(gel_app_shared_get(app, "dock"))
#define eina_obj_get_dock(obj) gel_app_get_dock(eina_obj_get_app(obj))

typedef struct _EinaDock EinaDock;

GtkWidget*
eina_dock_get_widget(GtkWidget *owner);

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *widget);

gboolean
eina_dock_remove_widget(EinaDock *self, gchar *id);

gboolean
eina_dock_switch_widget(EinaDock *iface, gchar *id);

G_END_DECLS

#endif // _EINA_DOCK
