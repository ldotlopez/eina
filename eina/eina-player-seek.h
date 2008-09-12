#ifndef _EINA_PLAYER_SEEK
#define _EINA_PLAYER_SEEK

#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblomo/player.h>

G_BEGIN_DECLS

#define EINA_PLAYER_TYPE_SEEK eina_player_seek_get_type()

#define EINA_PLAYER_SEEK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_PLAYER_TYPE_SEEK, EinaPlayerSeek))

#define EINA_PLAYER_SEEK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_PLAYER_TYPE_SEEK, EinaPlayerSeekClass))

#define EINA_PLAYER_IS_SEEK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_PLAYER_TYPE_SEEK))

#define EINA_PLAYER_IS_SEEK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_PLAYER_TYPE_SEEK))

#define EINA_PLAYER_SEEK_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_PLAYER_TYPE_SEEK, EinaPlayerSeekClass))

typedef struct
{
	GtkHScale parent;
} EinaPlayerSeek;

typedef struct
{
	GtkHScaleClass parent_class;
} EinaPlayerSeekClass;

GType eina_player_seek_get_type (void);

EinaPlayerSeek* eina_player_seek_new          (void);
void            eina_player_seek_updater_start(EinaPlayerSeek *self);
void            eina_player_seek_updater_stop (EinaPlayerSeek *self);

void eina_player_seek_set_lomo_player    (EinaPlayerSeek *self, LomoPlayer *lomo);
void eina_player_seek_set_current_label  (EinaPlayerSeek *self, GtkLabel *label);
void eina_player_seek_set_total_label    (EinaPlayerSeek *self, GtkLabel *label);
void eina_player_seek_set_remaining_label(EinaPlayerSeek *self, GtkLabel *label);

LomoPlayer* eina_player_seek_get_lomo_player    (EinaPlayerSeek *self);
GtkLabel*   eina_player_seek_get_current_label  (EinaPlayerSeek *self);
GtkLabel*   eina_player_seek_get_total_label    (EinaPlayerSeek *self);
GtkLabel*   eina_player_seek_get_remaining_label(EinaPlayerSeek *self);

G_END_DECLS

#endif /* _EINA_PLAYER_SEEK */
