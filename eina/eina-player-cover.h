#ifndef _EINA_PLAYER_COVER
#define _EINA_PLAYER_COVER

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_PLAYER_TYPE_COVE eina_player_cover_get_type()

#define EINA_PLAYER_COVE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_PLAYER_TYPE_COVE, EinaPlayerCover))

#define EINA_PLAYER_COVE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_PLAYER_TYPE_COVE, EinaPlayerCoverClass))

#define EINA_PLAYER_IS_COVE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_PLAYER_TYPE_COVE))

#define EINA_PLAYER_IS_COVE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_PLAYER_TYPE_COVE))

#define EINA_PLAYER_COVE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_PLAYER_TYPE_COVE, EinaPlayerCoverClass))

typedef struct {
	GtkImage parent;
} EinaPlayerCover;

typedef struct {
	GtkImageClass parent_class;
} EinaPlayerCoverClass;

GType eina_player_cover_get_type (void);

EinaPlayerCover* eina_player_cover_new (void);

void eina_player_reset(EinaPlayerCover* self);

G_END_DECLS
