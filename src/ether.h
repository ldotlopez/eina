#ifndef __EINA_ETHER_H__
#define __EINA_ETHER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_ETHER eina_ether_get_type()
#define EINA_ETHER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  EINA_TYPE_ETHER, EinaEther))
#define EINA_ETHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  EINA_TYPE_ETHER, EinaEtherClass))
#define EINA_IS_ETHER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  EINA_TYPE_ETHER))
#define EINA_IS_ETHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  EINA_TYPE_ETHER))
#define EINA_ETHER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  EINA_TYPE_ETHER, EinaEtherClass))

typedef struct _EinaEtherPrivate EinaEtherPrivate;
typedef struct {
	GObject parent;
	EinaEtherPrivate *priv;
} EinaEther;

typedef struct {
	GObjectClass parent_class;
	void (*signal_1) (EinaEther *self);
} EinaEtherClass;

GType eina_ether_get_type (void);
EinaEther* eina_ether_new (void);

G_END_DECLS

#endif
