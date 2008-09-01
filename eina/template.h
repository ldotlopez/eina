#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include <gel/gel.h>

typedef struct _EinaTemplate EinaTemplate;

gboolean template_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean template_exit(gpointer data);

/* * * * * * * * * * */
/* Public functions  */
/* * * * * * * * * * */
/* void template_do_something(EinaTemplate *self, ...); */

#endif
