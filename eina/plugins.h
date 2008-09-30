#ifndef _PLUGINS_H
#define _PLUGINS_H

#include <gel/gel.h>

typedef struct _EinaPlugins EinaPlugins;

gboolean eina_plugins_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean eina_plugins_exit(gpointer data);

void eina_plugins_show(EinaPlugins *self);
void eina_plugins_hide(EinaPlugins *self);

#endif
