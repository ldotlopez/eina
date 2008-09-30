#ifndef _EINA_TAGGER
#define _EINA_TAGGER

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_TAGGER eina_tagger_get_type()

#define EINA_TAGGER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_TAGGER, EinaTagger))

#define EINA_TAGGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_TAGGER, EinaTaggerClass))

#define EINA_IS_TAGGER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_TAGGER))

#define EINA_IS_TAGGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_TAGGER))

#define EINA_TAGGER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_TAGGER, EinaTaggerClass))

typedef struct {
	GtkEntry parent;
} EinaTagger;

typedef struct {
	GtkEntryClass parent_class;
} EinaTaggerClass;

GType eina_tagger_get_type (void);

EinaTagger* eina_tagger_new (void);

G_END_DECLS

#endif
