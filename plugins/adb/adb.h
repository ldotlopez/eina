/*
 * plugins/adb/adb.h 
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

#ifndef _EINA_ADB_H
#define _EINA_ADB_H

G_BEGIN_DECLS

#include "eina-adb.h"

#define gel_app_get_adb(app)        EINA_ADB(gel_app_shared_get(app, "adb"))
#define eina_plugin_get_adb(plugin) gel_app_get_adb(gel_plugin_get_app(plugin))
#define eina_obj_get_adb(obj)       gel_app_get_adb(eina_obj_get_app(obj))

G_END_DECLS

#endif // _EINA_ADB_H
