/*
 * recently.h
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

#ifndef _PLUGIN_RECENTLY_H
#define _PLUGIN_RECENTLY_H

#include <eina/eina-plugin.h>

G_BEGIN_DECLS

typedef struct _Recently Recently;

enum {
	RECENTLY_NO_ERROR = 0,
	RECENTLY_CANNOT_LOAD_ADB,
	RECENTLY_CANNOT_UNLOAD_ADB,
	RECENTLY_NO_ADB_OBJECT
};

GQuark recently_quark(void);

Recently *recently_new(GelApp *app, GError **error);
void      recently_free(Recently *self);

G_END_DECLS

#endif

