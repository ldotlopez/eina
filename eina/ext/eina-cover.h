#ifndef _EINA_COVER
#define _EINA_COVER

#include <glib-object.h>
#include <gtk/gtk.h>
#include <lomo/lomo-player.h>
#include <eina/art.h>

G_BEGIN_DECLS

#define EINA_TYPE_COVER eina_cover_get_type()

#define EINA_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_COVER, EinaCover))

#define EINA_COVER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_COVER, EinaCoverClass))

#define EINA_IS_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_COVER))

#define EINA_IS_COVER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_COVER))

#define EINA_COVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_COVER, EinaCoverClass))

typedef struct {
	GtkVBox parent;
} EinaCover;

typedef struct {
	GtkVBoxClass parent_class;
} EinaCoverClass;

GType eina_cover_get_type (void);

EinaCover* eina_cover_new (void);
void       eina_cover_set_renderer(EinaCover *self, GtkWidget *renderer);
void       eina_cover_set_art(EinaCover *self, Art *art);
void       eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo);
void       eina_cover_set_default_pixbuf(EinaCover *self, GdkPixbuf *pixbuf);
void       eina_cover_set_loading_pixbuf(EinaCover *self, GdkPixbuf *pixbuf);

G_END_DECLS

#endif /* _EINA_COVER */
