/*
 * eina/dbus.h
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

#ifndef _EINA_DBUS_H
#define _EINA_DBUS_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum { 
	EINA_DBUS_NO_ERROR = 0, 
	EINA_DBUS_CANNOT_GET_DBUS_PROXY
} EinaDBusError; 

G_END_DECLS

#endif

