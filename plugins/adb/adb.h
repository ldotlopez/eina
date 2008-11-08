#ifndef __ADB_H__
#define __ADB_H__

#include <sqlite3.h>
#include <glib.h>
#include <lomo/player.h>

G_BEGIN_DECLS

typedef struct Adb {
	sqlite3 *db;
} Adb;

Adb *adb_new(LomoPlayer *lomo, GError **error);
void adb_free(Adb *self);

G_END_DECLS

#endif
