#ifndef _PLAYER_H
#define _PLAYER_H

#include <gel/gel.h>
#include <eina/eina-cover.h>

G_BEGIN_DECLS

typedef struct _EinaPlayer EinaPlayer;

#define GEL_HUB_GET_PLAYER(hub)    gel_hub_shared_get(hub,"player")
#define EINA_BASE_GET_PLAYER(base) GEL_HUB_GET_PLAYER(((EinaBase *)base)->hub)

EinaCover*    eina_player_get_cover      (EinaPlayer* self);
GtkUIManager* eina_player_get_ui_manager (EinaPlayer* self);
GtkWindow*    eina_player_get_main_window(EinaPlayer* self);

G_END_DECLS

#endif // _PLAYER_H
