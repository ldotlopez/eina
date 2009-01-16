/*
 * eina/artwork.h
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

#ifndef _ARTWORK_H
#define _ARTWORK_H

#include <eina/eina-obj.h>
#include <eina/eina-artwork.h>

#define GEL_APP_GET_ARTWORK(app)    EINA_ARTWORK(gel_app_shared_get(app, "artwork"))
#define EINA_OBJ_GET_ARTWORK(obj)   GEL_APP_GET_ARTWORK(eina_obj_get_app(obj))

#endif
