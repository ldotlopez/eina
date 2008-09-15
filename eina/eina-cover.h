#ifndef _EINA_COVER
#define _EINA_COVER

#include <glib-object.h>
#include <gtk/gtk.h>
#include <liblomo/player.h>

G_BEGIN_DECLS

#define EINA_TYPE_COVER eina_cover_get_type()

#define EINA_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_COVER, EinaCover))

#define EINA_COVER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_COVER, EinaCoverClass))

#define EINA_IS_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_COVER))

#define EINA_IS_COVE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_COVER))

#define EINA_COVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_COVER, EinaCoverClass))

typedef struct {
	GtkImage parent;
} EinaCover;

typedef struct {
	GtkImageClass parent_class;
} EinaCoverClass;

typedef void (*EinaCoverBackendFunc)(EinaCover *self, const LomoStream *stream, gpointer data);
typedef void (*EinaCoverBackendCancelFunc)(EinaCover *self, gpointer data);

GType eina_cover_get_type (void);

EinaCover* eina_cover_new (void);

void        eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo);
LomoPlayer* eina_cover_get_lomo_player(EinaCover *self);

void eina_cover_set_default_cover(EinaCover *self, gchar *filename);
gchar *eina_cover_get_default_cover(EinaCover *self);

void eina_cover_add_backend(EinaCover *self, const gchar *name,
	EinaCoverBackendFunc callback, EinaCoverBackendCancelFunc cancel,
	gpointer data);
void eina_cover_delete_backend(EinaCover *self, const gchar *name);

void eina_cover_provider_fail   (EinaCover *self);
void eina_cover_provider_success(EinaCover *self, GType type, gpointer data);

G_END_DECLS

#endif // _EINA_COVER
