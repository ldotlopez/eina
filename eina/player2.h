#ifndef _PLAYER2_H
#define _PLAYER2_H

#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_PLAYER(p)           ((EinaPlayer *) p)
#define GEL_APP_GET_PLAYER(app)  EINA_PLAYER(gel_app_shared_get(app, "player"))
#define EINA_OBJ_GET_PLAYER(obj) GEL_APP_GET_PLAYER(eina_obj_get_app(obj))

typedef struct _EinaPlayer EinaPlayer;

GtkUIManager* eina_player_get_ui_manager (EinaPlayer* self);
GtkWindow*    eina_player_get_main_window(EinaPlayer* self);
void          eina_player_add_widget     (EinaPlayer* self, GtkWidget *widget);
void          eina_player_remove_widget  (EinaPlayer* self, GtkWidget *widget);

G_END_DECLS

#endif
