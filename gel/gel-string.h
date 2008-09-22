#ifndef _GEL_STRING
#define _GEL_STRING

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GelString GelString;
typedef struct _GelStringPriv GelStringPriv;

struct _GelString {
	gchar *str;
	GelStringPriv *priv;
};

GelString *gel_string_new(gchar *str);

void gel_string_ref  (GelString *self);
void gel_string_unref(GelString *self);

G_END_DECLS

#endif // _GEL_STRING