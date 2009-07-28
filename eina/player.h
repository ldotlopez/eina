/*
 * eina/player.h
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EINA_PLAYER_H
#define _EINA_PLAYER_H

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
GtkContainer* eina_player_get_cover_container(EinaPlayer* self);
void          eina_player_add_widget     (EinaPlayer* self, GtkWidget *widget);
void          eina_player_remove_widget  (EinaPlayer* self, GtkWidget *widget);
void          eina_player_set_persistent (EinaPlayer* self, gboolean value);
gboolean      eina_player_get_persistent (EinaPlayer *self);

G_END_DECLS

#endif
