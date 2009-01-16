/*
 * eina/plugins.h
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

#ifndef _PLUGINS_H
#define _PLUGINS_H

#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

typedef struct _EinaPlugins EinaPlugins;

#define EINA_PLUGINS(p)           ((EinaPlugins *) p)
#define GEL_APP_GET_PLUGINS(app)  gel_app_shared_get(app, "plugins")
#define EINA_OBJ_GET_PLUGINS(obj) GEL_APP_GET_PLUGINS(eina_obj_get_app(obj))

G_END_DECLS

#endif
