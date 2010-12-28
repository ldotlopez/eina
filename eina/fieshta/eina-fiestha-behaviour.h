#ifndef _EINA_FIESHTA_BEHAVIOUR
#define _EINA_FIESHTA_BEHAVIOUR

#include <glib-object.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_FIESHTA_BEHAVIOUR eina_fieshta_behaviour_get_type()

#define EINA_FIESHTA_BEHAVIOUR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviour)) 
#define EINA_FIESHTA_BEHAVIOUR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST  ((klass), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviourClass)) 
#define EINA_IS_FIESHTA_BEHAVIOUR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR)) 
#define EINA_IS_FIESHTA_BEHAVIOUR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE  ((klass), EINA_TYPE_FIESHTA_BEHAVIOUR)) 
#define EINA_FIESHTA_BEHAVIOUR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviourClass))

typedef struct _EinaFieshtaBehaviourPrivate EinaFieshtaBehaviourPrivate;
typedef struct {
	GObject parent;
	EinaFieshtaBehaviourPrivate *priv;
} EinaFieshtaBehaviour;

typedef struct {
	GObjectClass parent_class;
} EinaFieshtaBehaviourClass;

typedef enum {
	EINA_FIESHTA_BEHAVIOUR_OPTION_NONE     = 0,
	EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF  = 1,
	EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW = 1 << 1,
	EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT   = 1 << 2,
	EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE   = 1 << 3,
	EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT  = 
		EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF  |
		EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW |
		EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT   |
		EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE
} EinaFieshtaBehaviourOptions;

GType eina_fieshta_behaviour_get_type (void);

EinaFieshtaBehaviour* eina_fieshta_behaviour_new (LomoPlayer *lomo, EinaFieshtaBehaviourOptions options);

LomoPlayer *
eina_fieshta_behaviour_get_lomo_player(EinaFieshtaBehaviour *self);

EinaFieshtaBehaviourOptions
eina_fieshta_behaviour_get_options(EinaFieshtaBehaviour *self);

void
eina_fieshta_behaviour_set_options(EinaFieshtaBehaviour *self, EinaFieshtaBehaviourOptions options);

G_END_DECLS

#endif /* _EINA_FIESHTA_BEHAVIOUR */
