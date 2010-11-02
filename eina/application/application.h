/*
 * eina/application/application.h
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

#ifndef _APPLICATION
#define _APPLICATION

#include <eina/eina-plugin.h>
#include <gel/gel-ui.h>

// Accessors
#define gel_plugin_engine_get_application(engine) gel_plugin_engine_get_interface(engine, "application")
#define eina_plugin_get_application(plugin)       gel_plugin_engine_get_application(gel_plugin_get_engine(plugin))

// Plugin API
#define eina_plugin_get_application_window(plugin) gel_ui_application_get_window(eina_plugin_get_application(plugin))

#endif
