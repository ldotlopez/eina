#ifndef _PLAYER_H
#define _PLAYER_H

#include <gel/gel.h>
#include <eina/eina-cover.h>

G_BEGIN_DECLS

typedef struct _EinaPlayer EinaPlayer;

EinaCover    *eina_player_get_cover(EinaPlayer *self);
GtkUIManager *eina_player_get_ui_manager(EinaPlayer *self);
GtkWindow    *eina_player_get_main_window(EinaPlayer *self);

G_END_DECLS

#endif // _PLAYER_H
