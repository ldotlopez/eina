#ifndef _PLAYER_H
#define _PLAYER_H

#include <gel/gel.h>
#include <eina/base.h>
#include <eina/eina-cover.h>

#define EINA_PLAYER(p)             ((EinaPlayer *) p)
#define GEL_HUB_GET_PLAYER(hub)    EINA_PLAYER(gel_hub_shared_get(hub, "player"))
#define EINA_BASE_GET_PLAYER(base) GEL_HUB_GET_PLAYER(EINA_BASE(base)->hub)

G_BEGIN_DECLS

typedef struct _EinaPlayer EinaPlayer;

EinaCover*    eina_player_get_cover      (EinaPlayer* self);
GtkUIManager* eina_player_get_ui_manager (EinaPlayer* self);
GtkWindow*    eina_player_get_main_window(EinaPlayer* self);
void          eina_player_add_widget     (EinaPlayer* self, GtkWidget *widget);
void          eina_player_remove_widget  (EinaPlayer* self, GtkWidget *widget);

G_END_DECLS

#endif // _PLAYER_H
