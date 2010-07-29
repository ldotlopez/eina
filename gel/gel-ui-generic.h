/*
 * gel/gel-ui-generic.h
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

#ifndef _GEL_UI_GENERIC
#define _GEL_UI_GENERIC

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEL_UI_TYPE_GENERIC gel_ui_generic_get_type()

#define GEL_UI_GENERIC(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_UI_TYPE_GENERIC, GelUIGeneric))

#define GEL_UI_GENERIC_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GEL_UI_TYPE_GENERIC, GelUIGenericClass))

#define GEL_UI_IS_GENERIC(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_UI_TYPE_GENERIC))

#define GEL_UI_IS_GENERIC_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_UI_TYPE_GENERIC))

#define GEL_UI_GENERIC_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_UI_TYPE_GENERIC, GelUIGenericClass))

typedef struct {
	GtkBox parent;
} GelUIGeneric;

typedef struct {
	GtkBoxClass parent_class;
} GelUIGenericClass;

GType gel_ui_generic_get_type (void);

GtkWidget  *gel_ui_generic_new (gchar *xml_string);
GtkBuilder *gel_ui_generic_get_builder(GelUIGeneric *self);

#define gel_ui_generic_get_typed(object,type_macro,name) \
	type_macro(gtk_builder_get_object(gel_ui_generic_get_builder(GEL_UI_GENERIC(object)),name))

G_END_DECLS

#endif /* _GEL_UI_GENERIC */
