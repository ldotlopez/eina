#ifndef __COVER_H__
#define __COVER_H__

#include "libghub/ghub.h"

typedef struct _EinaCover EinaCover;

gboolean cover_init(GHub *hub, gint *argc, gchar ***argv);
gboolean cover_exit (gpointer data);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void cover_do_something(EinaCover *self, ...); */

#endif
