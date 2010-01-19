/*
 * eina/template.h
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

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

// Gel library (GLib extension library)
#include <gel/gel.h>
// GelIO library (GIO extension library)
#include <gel/gel-io.h>
// GelUI library (Gtk extension library)
#include <gel/gel-ui.h>
// EinaObj is a base pseudo-class which do things more easy
#include <eina/eina-obj.h>

G_BEGIN_DECLS

// Do some defines to make easy to access your plugin from other plugins
#define EINA_TEMPLATE(s)           ((EinaTemplate *) s)
#define GEL_APP_GET_TEMPLATE(app)  EINA_TEMPLATE(gel_app_shared_get(app,"template"))
#define EINA_OBJ_GET_TEMPLATE(obj) GEL_APP_GET_TEMPLATE(eina_obj_get_app(obj))

// Define an opaque struct if you want to expose an API to the world
typedef struct _EinaTemplate EinaTemplate;

// --
// Define your API here
//
// void template_do_something(EinaTemplate *self, ...); 
// --

G_END_DECLS

#endif
