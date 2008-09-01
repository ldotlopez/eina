#ifndef __NOTIFY_H__
#define __NOTIFY_H__

#ifndef MY_DOMAIN
#define MY_DOMAIN "Eina::Notify"
#endif

#include <glib.h>
#include "libghub/ghub.h"

typedef struct _EinaNotify EinaNotify;

gboolean eina_notify_init(GelHub *hub, gint argc, gchar *argv[]);
gboolean eina_notify_exit(EinaNotify *self);

#endif
