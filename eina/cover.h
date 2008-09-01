#ifndef __COVER_H__
#define __COVER_H__

#include <gel/gel.h>

typedef struct _EinaCover EinaCover;

gboolean cover_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean cover_exit (gpointer data);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void cover_do_something(EinaCover *self, ...); */

#endif
