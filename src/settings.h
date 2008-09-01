#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "libghub/ghub.h"
#include "class-conf-file.h"

gboolean settings_init(GHub *hub, gint *argc, gchar ***argv);
gboolean settings_exit(gpointer data);

#endif
