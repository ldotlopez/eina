/*
 * plugins/dock/dock.h
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

#ifndef _DOCK_H
#define _DOCK_H

#include <eina/eina-plugin2.h>
#include <eina/dock/eina-dock.h>

#define gel_plugin_engine_get_dock(engine) gel_plugin_engine_get_interface(engine, "dock")
#define gel_plugin_get_dock(plugin)        gel_plugin_engine_get_dock(gel_plugin_get_engine(plugin))
#define eina_plugin_get_dock(plugin)       gel_plugin_get_dock(plugin)

#define EINA_DOCK_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.dock"
#define EINA_DOCK_ORDER_KEY    "page-order"
#define EINA_DOCK_EXPANDED_KEY "expanded"
#define EINA_DOCK_WINDOW_H_KEY "window-height"
#define EINA_DOCK_WINDOW_W_KEY "window-width"

gboolean
eina_plugin_add_dock_widget(EinaPlugin *plugin, gchar *id, GtkWidget *label, GtkWidget *widget);
gboolean
eina_plugin_switch_dock_widget(EinaPlugin *plugin, gchar *id);
gboolean
eina_plugin_remove_dock_widget(EinaPlugin *plugin, GtkWidget *widget);
gboolean
eina_plugin_remove_dock_widget_by_id(EinaPlugin *plugin, gchar *id);

#endif

