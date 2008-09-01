#define MY_DOMAIN "Eina::Ether"
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
#include "ether.h"
#include <glib/gprintf.h>

G_DEFINE_TYPE (EinaEther, eina_ether, G_TYPE_OBJECT);

/*
 * Private
 */
/* Define the private members of class */
struct _EinaEtherPrivate {
	/*
	 * gpointer reserved_1;
	 * gpointer reserved_2;
	 * ...
	 */
};
/* Macro to get private struct of class */
#define EINA_ETHER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_ETHER, EinaEtherPrivate))

/*
 * Signals
 */
typedef enum {
	SIGNAL_1,
	LAST_SIGNAL
} EinaEtherSignalType;
static guint eina_ether_signals[LAST_SIGNAL] = { 0 };

static void eina_ether_finalize (GObject *object);
static void eina_ether_class_init (EinaEtherClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = eina_ether_finalize;

	g_printf("%s\n", __FUNCTION__);

	/* Create the class: insert signals, properties, etc ... */
    eina_ether_signals[SIGNAL_1] = g_signal_new ("signal-1",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (EinaEtherClass, signal_1),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE,
        0);
}

static void eina_ether_init (EinaEther *self) {
	g_printf("%s\n", __FUNCTION__);
	/* Init that instance */

	/* Create private members */
	self->priv = g_new0(EinaEtherPrivate, 1);
}

EinaEther *eina_ether_new (void) {
	g_printf("%s\n", __FUNCTION__);
	return g_object_new (EINA_TYPE_ETHER, NULL);
}

static void eina_ether_finalize (GObject *object) {
	EinaEther *self = EINA_ETHER(object);
	
	g_printf("%s\n", __FUNCTION__);
	g_free(self->priv);

	/* Free allocated memory and resource for that instance */
	/* ... */

	/* Call parent's finalize */
	G_OBJECT_CLASS (eina_ether_parent_class)->finalize (object);
}

