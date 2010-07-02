/*
 * gel/gel-ui-plugin-manager.h
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

#ifndef __GEL_UI_PLUGIN_MANAGER_H__
#define __GEL_UI_PLUGIN_MANAGER_H__

#include <gel/gel-ui.h>

G_BEGIN_DECLS

#define GEL_UI_TYPE_PLUGIN_MANAGER            gel_ui_plugin_manager_get_type()
#define GEL_UI_PLUGIN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_UI_TYPE_PLUGIN_MANAGER, GelUIPluginManager))
#define GEL_UI_PLUGIN_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEL_UI_TYPE_PLUGIN_MANAGER, GelUIPluginManagerClass))
#define GEL_UI_IS_PLUGIN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_UI_TYPE_PLUGIN_MANAGER))
#define GEL_UI_IS_PLUGIN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEL_UI_TYPE_PLUGIN_MANAGER))
#define GEL_UI_PLUGIN_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEL_UI_TYPE_PLUGIN_MANAGER, GelUIPluginManagerClass))

typedef struct {
	GtkWidget parent;
} GelUIPluginManager;

typedef struct {
	GtkWidgetClass parent_class;
} GelUIPluginManagerClass;

enum {
	GEL_UI_PLUGIN_MANAGER_RESPONSE_INFO = 1
};

GType gel_ui_plugin_manager_get_type (void);

GelUIPluginManager *
gel_ui_plugin_manager_new (GelApp *app);

GelPluginEngine *
gel_ui_plugin_manager_get_engine(GelUIPluginManager *self);

GelPlugin *
gel_ui_plugin_manager_get_selected_plugin(GelUIPluginManager *self);

G_END_DECLS

#endif
