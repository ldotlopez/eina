#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include "libghub/ghub.h"

typedef struct _EinaLog EinaLog;

gboolean log_init(GHub *hub, gint *argc, gchar ***argv);
gboolean log_exit(gpointer data);

#endif
