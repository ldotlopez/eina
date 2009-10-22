/*
 * eina/window.h
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

#ifndef _WINDOW_H
#define _WINDOW_H

#include <eina/ext/eina-window.h>

G_BEGIN_DECLS

#define gel_app_get_window(app)  EINA_WINDOW(gel_app_shared_get(app, "window"))
#define eina_obj_get_window(obj) gel_app_get_window(eina_obj_get_app(obj))

#define GEL_APP_GET_WINDOW(app)  gel_app_get_window(app)
#define EINA_OBJ_GET_WINDOW(obj) eina_obj_get_window(obj)

#define gel_app_get_window(app)  EINA_WINDOW(gel_app_shared_get(app, "window"))
#define eina_obj_get_window(obj) GEL_APP_GET_WINDOW(eina_obj_get_app(obj))

enum {
	EINA_WINDOW_NO_ERROR = 0,
	EINA_WINDOW_ERROR_REGISTER,
	EINA_WINDOW_ERROR_NOT_FOUND
};

G_END_DECLS

#endif
