#ifndef __VOGON_H__
#define __VOGON_H__

#include "libghub/ghub.h"

typedef struct _EinaVogon EinaVogon;

gboolean vogon_init(GHub *hub, gint *argc, gchar ***argv);
gboolean vogon_exit(gpointer data);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void vogon_do_something(EinaVogon *self, ...); */

#endif
