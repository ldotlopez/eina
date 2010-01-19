/*
 * eina/vogon.h
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

#ifndef _EINA_VOGON_H
#define _EINA_VOGON_H

#include <eina/eina-plugin.h>

#define EINA_VOGON(p)           ((EinaVogon *) p)
#define GEL_APP_GET_VOGON(app)  EINA_VOGON(gel_app_shared_get(app, "vogon"))
#define EINA_OBJ_GET_VOGON(obj) GEL_APP_GET_VOGON(eina_obj_get_app(obj))

typedef enum {
	EINA_VOGON_NO_ERROR = 0, 
	EINA_VOGON_ERROR_OSX_QUARTZ,
	EINA_VOGON_ERROR_NO_STATUS_ICON,
	EINA_VOGON_ERROR_NO_SETTINGS_OBJECT
} EinaVogonError;

typedef struct _EinaVogon EinaVogon;

GtkStatusIcon*
eina_vogon_get_status_icon(EinaVogon *self);

GtkUIManager*
eina_vogon_get_ui_manager(EinaVogon *self);

#endif // _EINA_VOGON_H

