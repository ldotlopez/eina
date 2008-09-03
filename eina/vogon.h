#ifndef _VOGON_H
#define _VOGON_H

#include <gel/gel.h>

typedef struct _EinaVogon EinaVogon;

gboolean
eina_vogon_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean
eina_vogon_exit(gpointer data);

#endif // _VOGON_H
