#ifndef _EINA_SEEK
#define _EINA_SEEK

#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblomo/player.h>

G_BEGIN_DECLS

#define EINA_TYPE_SEEK eina_seek_get_type()

#define EINA_SEEK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_SEEK, EinaSeek))

#define EINA_SEEK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_SEEK, EinaSeekClass))

#define EINA_PLAYER_IS_SEEK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_SEEK))

#define EINA_PLAYER_IS_SEEK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_SEEK))

#define EINA_SEEK_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_SEEK, EinaSeekClass))

typedef struct
{
	GtkHScale parent;
} EinaSeek;

typedef struct
{
	GtkHScaleClass parent_class;
} EinaSeekClass;

GType eina_seek_get_type (void);

EinaSeek* eina_seek_new          (void);
void            eina_seek_updater_start(EinaSeek *self);
void            eina_seek_updater_stop (EinaSeek *self);

void eina_seek_set_lomo_player    (EinaSeek *self, LomoPlayer *lomo);
void eina_seek_set_current_label  (EinaSeek *self, GtkLabel *label);
void eina_seek_set_total_label    (EinaSeek *self, GtkLabel *label);
void eina_seek_set_remaining_label(EinaSeek *self, GtkLabel *label);

LomoPlayer* eina_seek_get_lomo_player    (EinaSeek *self);
GtkLabel*   eina_seek_get_current_label  (EinaSeek *self);
GtkLabel*   eina_seek_get_total_label    (EinaSeek *self);
GtkLabel*   eina_seek_get_remaining_label(EinaSeek *self);

G_END_DECLS

#endif /* _EINA_SEEK */
