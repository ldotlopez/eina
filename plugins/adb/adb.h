/*
 * plugins/adb/adb.h
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

#ifndef __ADB_H__
#define __ADB_H__

#include <sqlite3.h>
#include <glib.h>
#include <lomo/player.h>
#include "common.h"

G_BEGIN_DECLS

enum {
	ADB_NO_ERROR = 0,
	ADB_CANNOT_GET_LOMO,
	ADB_CANNOT_REGISTER_OBJECT
};

typedef struct Adb {
	sqlite3 *db;
} Adb;

#define GEL_APP_GET_ADB(app) ((Adb *) gel_app_shared_get(app, "adb"))

GQuark adb_quark(void);
Adb *adb_new(LomoPlayer *lomo, GError **error);
void adb_free(Adb *self);

G_END_DECLS

#endif
