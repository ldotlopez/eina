/*
 * eina/adb/eina-adb-plugin.h
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

#ifndef __EINA_ADB_PLUGIN_H__
#define __EINA_ADB_PLUGIN_H__

#include <eina/ext/eina-extension.h>
#include <eina/adb/eina-adb.h>

G_BEGIN_DECLS

EinaAdb *
eina_application_get_adb(EinaApplication *application);

/**
 * EinaAdbPluginError:
 * @EINA_ADB_PLUGIN_ERROR_UNABLE_TO_SET_DB_FILE: Database is unusable
 * @EINA_ADB_PLUGIN_ERROR_CANNOT_REGISTER_INTERFACE: Unable to register adb
 *                                                   into application
 * Errors associated with #EinaAdbPlugin
 */
typedef enum {
	EINA_ADB_PLUGIN_ERROR_UNABLE_TO_SET_DB_FILE = 1,
	EINA_ADB_PLUGIN_ERROR_CANNOT_REGISTER_INTERFACE
} EinaAdbPluginError;

G_END_DECLS

#endif // __EINA_ADB_PLUGIN_H__
