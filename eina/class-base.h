#ifndef _EINA_BASE
#define _EINA_BASE

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_BASE eina_base_get_type()

#define EINA_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_BASE, EinaBase))

#define EINA_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_BASE, EinaBaseClass))

#define EINA_IS_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_BASE))

#define EINA_IS_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_BASE))

#define EINA_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_BASE, EinaBaseClass))

typedef struct {
  GObject parent;
} EinaBase;

typedef struct {
  GObjectClass parent_class;
} EinaBaseClass;

GType eina_base_get_type (void);

EinaBase* eina_base_new (void);

G_END_DECLS

#endif /* _EINA_BASE */

