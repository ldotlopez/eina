#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include "libghub/ghub.h"

typedef struct _EinaTemplate EinaTemplate;

gboolean template_init(GHub *hub, gint *argc, gchar ***argv);
gboolean template_exit(gpointer data);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void template_do_something(EinaTemplate *self, ...); */

#endif
