#ifndef _EINA_PLAYER_VOLUME
#define _EINA_PLAYER_VOLUME

#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblomo/player.h>

G_BEGIN_DECLS

#define EINA_PLAYER_TYPE_VOLUME eina_player_volume_get_type()

#define EINA_PLAYER_VOLUME(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_PLAYER_TYPE_VOLUME, EinaPlayerVolume))

#define EINA_PLAYER_VOLUME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_PLAYER_TYPE_VOLUME, EinaPlayerVolumeClass))

#define EINA_PLAYER_IS_VOLUME(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_PLAYER_TYPE_VOLUME))

#define EINA_PLAYER_IS_VOLUME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_PLAYER_TYPE_VOLUME))

#define EINA_PLAYER_VOLUME_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_PLAYER_TYPE_VOLUME, EinaPlayerVolumeClass))

struct _GtkVolumeButton
{
	GtkScaleButton scale;
};

typedef struct
{
	GtkVolumeButton parent;
} EinaPlayerVolume;

typedef struct
{
	GtkVolumeButtonClass parent_class;
} EinaPlayerVolumeClass;

GType eina_player_volume_get_type (void);

EinaPlayerVolume *eina_player_volume_new       (LomoPlayer *lomo);
GtkWidget        *eina_player_volume_get_widget(EinaPlayerVolume *self);

G_END_DECLS

#endif /* _EINA_PLAYER_VOLUME */

