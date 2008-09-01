#ifndef _LOG_H
#define _LOG_H

#include <gel/gel.h>

G_BEGIN_DECLS

typedef struct _EinaLog EinaLog;

gboolean log_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean log_exit(gpointer data);

G_END_DECLS

#endif // _LOG_H
