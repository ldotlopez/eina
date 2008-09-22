#ifndef _EINA_VOLUME
#define _EINA_VOLUME

#include <glib-object.h>
#include <gtk/gtk.h>
#include <lomo/player.h>

G_BEGIN_DECLS

#define EINA_TYPE_VOLUME eina_volume_get_type()

#define EINA_VOLUME(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_VOLUME, EinaVolume))

#define EINA_VOLUME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_VOLUME, EinaVolumeClass))

#define EINA_IS_VOLUME(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_VOLUME))

#define EINA_IS_VOLUME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_VOLUME))

#define EINA_VOLUME_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_VOLUME, EinaVolumeClass))

#if !GTK_CHECK_VERSION(2,12,10)
// Al least on 2.12.9 GtkVolumeButton headers don't declare this time making
// impossible to subclass it
struct _GtkVolumeButton
{
	GtkScaleButton scale;
};
#endif

typedef struct
{
	GtkVolumeButton parent;
} EinaVolume;

typedef struct
{
	GtkVolumeButtonClass parent_class;
} EinaVolumeClass;

GType eina_volume_get_type (void);

EinaVolume *eina_volume_new(void);

void        eina_volume_set_lomo_player(EinaVolume *self, LomoPlayer *player);
LomoPlayer *eina_volume_get_lomo_player(EinaVolume *self);

G_END_DECLS

#endif /* _EINA_VOLUME */

