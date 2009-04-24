/*
 * eina/eina-lomo.h
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

#ifndef _EINA_LOMO_H
#define _EINA_LOMO_H

#include <gel/gel.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

typedef enum { 
	EINA_LOMO_NO_ERROR = 0, 
	EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE, 
	EINA_LOMO_ERROR_CANNOT_SET_SHARED, 
	EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE 
} EinaLomoError; 

#define EINA_BASE_GET_LOMO(base) GEL_HUB_GET_LOMO(((EinaBase *)base)->hub)
#define GEL_APP_GET_LOMO(app)    ((LomoPlayer *) gel_app_shared_get(app,"lomo"))
#define EINA_OBJ_GET_LOMO(obj)   GEL_APP_GET_LOMO(eina_obj_get_app(obj))

G_END_DECLS

#endif // _EINA_LOMO_H

