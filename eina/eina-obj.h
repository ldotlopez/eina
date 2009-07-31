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
#include <lomo/lomo-player.h>
#include <gel/gel.h>
#include <eina/lomo.h>

G_BEGIN_DECLS

typedef struct EinaObj {
	GelPlugin  *plugin;
	gchar      *name;
	LomoPlayer *lomo;
	GtkBuilder *ui;
} EinaObj;

typedef enum {
	EINA_OBJ_NONE   = 0x0000000,
	EINA_OBJ_GTK_UI = 0x0000001,
	EINA_OBJ_ALL    = EINA_OBJ_GTK_UI
} EinaObjFlag;

typedef struct {
	gchar *name;
	guint  index;
	GType  type;
} EinaObjObject;

gboolean eina_obj_init(EinaObj *self, GelPlugin *plugin, gchar *name, EinaObjFlag flags, GError **error);
void     eina_obj_fini(EinaObj *self);

void     eina_obj_strip(EinaObj *self, EinaObjFlag flags);

// Prefered way of access internals
#define EINA_OBJ(s)    ((EinaObj *)s)
#define eina_obj_get_app(self)    gel_plugin_get_app(eina_obj_get_plugin(self))
#define eina_obj_get_plugin(self) EINA_OBJ(self)->plugin
#define eina_obj_get_lomo(self)   EINA_OBJ(self)->lomo
#define eina_obj_get_ui(self)     EINA_OBJ(self)->ui

#define eina_obj_get_object(self,name)     gtk_builder_get_object(eina_obj_get_ui(self),name)
#define eina_obj_get_typed(self,type,name) type(eina_obj_get_object(self,name))
#define eina_obj_get_widget(self,name)     eina_obj_get_typed(self,GTK_WIDGET,name)

#define eina_obj_get_resource(self,type,resource) gel_plugin_get_resource(eina_obj_get_plugin(EINA_OBJ(self)), type, resource)

G_END_DECLS

#endif
