#ifndef __ADB_H__
#define __ADB_H__

#include <gel/gel.h>

typedef struct _EinaAdb EinaAdb;

gboolean adb_init(GelHub *hub, gint argc, gchar *argv[]);
gboolean adb_exit(EinaAdb *self);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void adb_do_something(EinaAdb *self, ...); */

#endif
