/*
 * eina/settings.h
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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <gel/gel.h>
#include <eina/eina-obj.h>
// Included for direct access to eina_conf_*, but this could be in
// eina-plugin.h
#include <eina/ext/eina-conf.h>

G_BEGIN_DECLS

#define EINA_SETTINGS(p)           ((EinaConf *) p)
#define GEL_APP_GET_SETTINGS(app)  EINA_SETTINGS(gel_app_shared_get(app,"settings"))
#define EINA_OBJ_GET_SETTINGS(obj) GEL_APP_GET_SETTINGS(eina_obj_get_app(obj))

#define gel_app_get_settings(app)  EINA_SETTINGS(gel_app_shared_get(app,"settings"))
#define eina_obj_get_settings(obj) GEL_APP_GET_SETTINGS(eina_obj_get_app(obj))

typedef enum {
	EINA_SETTINGS_NO_ERROR,
	EINA_SETTINGS_CANNOT_CREATE_CONFIG_DIR,
	EINA_SETTINGS_CONF_OBJECT_NOT_FOUND
} EinaSettingsError;

G_END_DECLS

#endif

