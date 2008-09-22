#ifndef _PLAYER_H
#define _PLAYER_H

#include <gel/gel.h>
#include <eina/eina-cover.h>

G_BEGIN_DECLS

typedef struct _EinaPlayer EinaPlayer;

gboolean   eina_player_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean   eina_player_exit(gpointer data);

EinaCover *eina_player_get_cover(EinaPlayer *self);

G_END_DECLS

#endif // _PLAYER_H
