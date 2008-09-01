#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <gel/gel.h>
#include "class-conf-file.h"

gboolean settings_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean settings_exit(gpointer data);

#endif
