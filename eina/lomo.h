#ifndef __EINA_LOMO_H__
#define __EINA_LOMO_H__

#include "libghub/ghub.h"

gboolean eina_lomo_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean eina_lomo_fini(gpointer data);

#endif
