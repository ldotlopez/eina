#ifndef __ADB_H__
#define __ADB_H__

#ifndef MY_DOMAIN
#define MY_DOMAIN "Eina::Adb"
#endif

#include "libghub/ghub.h"

typedef struct _EinaAdb EinaAdb;

gboolean adb_init(GHub *hub, gint argc, gchar *argv[]);
gboolean adb_exit(EinaAdb *self);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void adb_do_something(EinaAdb *self, ...); */

#endif
