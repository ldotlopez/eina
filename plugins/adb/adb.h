#ifndef __ADB_H__
#define __ADB_H__

#include <sqlite3.h>
#include <glib.h>
#include <lomo/player.h>

G_BEGIN_DECLS

typedef struct Adb {
	sqlite3 *db;
} Adb;

enum {
	ADB_NO_ERROR = 0,
	ADB_CANNOT_GET_LOMO,
	ADB_CANNOT_REGISTER_OBJECT
};

GQuark adb_quark(void);

Adb *adb_new(LomoPlayer *lomo, GError **error);
void adb_free(Adb *self);

G_END_DECLS

#endif
