/*
 * eina/eina-obj.h
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

#ifndef _EINA_OBJ
#define _EINA_OBJ

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <lomo/player.h>
#include <gel/gel.h>
#include <eina/lomo.h>

G_BEGIN_DECLS

typedef struct EinaObj {
	gchar       *name;
	GelApp      *app;
	LomoPlayer  *lomo;
	GtkBuilder  *ui;
} EinaObj;

typedef enum {
	EINA_OBJ_NONE,
	EINA_OBJ_GTK_UI
} EinaObjFlag;

gboolean eina_obj_init(EinaObj *self, GelApp *hub, gchar *name, EinaObjFlag flags, GError **error);
void     eina_obj_fini(EinaObj *self);

// Automatic loading of plugins if needed
gpointer eina_obj_require  (EinaObj *self, gchar *plugin_name, GError **error);
gboolean eina_obj_unrequire(EinaObj *self, gchar *plugin_name, GError **error);

// Prefered way of access internals
#define EINA_OBJ(s)    ((EinaObj *)s)
#define eina_obj_get_app(self)  EINA_OBJ(self)->app
#define eina_obj_get_lomo(self) EINA_OBJ(self)->lomo
#define eina_obj_get_ui(self)   EINA_OBJ(self)->ui

#define eina_obj_get_object(self,name)     gtk_builder_get_object(eina_obj_get_ui(self),name)
#define eina_obj_get_typed(self,type,name) type(eina_obj_get_object(self,name))
#define eina_obj_get_widget(self,name)     eina_obj_get_typed(self,GTK_WIDGET,name)

// Facility macros, not deprecated but not recomended
/*
#define LOMO(s)        EINA_OBJ(s)->lomo
#define APP(s)         EINA_OBJ(s)->app
#define UI(s)          EINA_OBJ(s)->ui
#define W(s,n)         W_TYPED(s,GTK_WIDGET,n)
#define W_TYPED(s,t,n) t(gtk_builder_get_object(UI(s),n))
#define OBJ(s,n)       G_OBJECT(gtk_builder_get_object(UI(s),n))
*/

G_END_DECLS

#endif
