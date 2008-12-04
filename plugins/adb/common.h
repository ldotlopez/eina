#ifndef _ADB_COMMON_H
#define _ADB_COMMON_H
#include <glib.h>
#include "adb.h"

G_BEGIN_DECLS

gboolean
adb_exec_querys(Adb *self, const gchar **querys, gint *success, GError **error);

gboolean
adb_set_variable(Adb *self, const gchar *variable, GType type, gpointer value);

G_END_DECLS

#endif // _ADB_COMMON_H
