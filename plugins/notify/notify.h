#ifndef _NOTIFY_H
#define _NOTIFY_H

#include <glib.h>
#include <gel/gel.h>

G_BEGIN_DECLS

typedef struct _EinaNotify EinaNotify;

gboolean eina_notify_init(GelHub *hub, gint argc, gchar *argv[]);
gboolean eina_notify_exit(EinaNotify *self);

G_END_DECLS

#endif // _NOTIFY_H
