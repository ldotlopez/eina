/*
 * eina/player.h
 *
 * Copyright (C) 2004-2010 Eina
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

#ifndef _PLAYER_H
#define _PLAYER_H

#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_PLAYER(obj)               ((EinaPlayer *) obj)
#define gel_app_get_player(app)        EINA_PLAYER(gel_app_shared_get(app, "player"))
#define eina_plugin_get_player(plugin) gel_app_get_player(gel_plugin_get_app(plugin))
#define eina_obj_get_player(obj)       gel_app_get_player(eina_obj_get_app(obj))

#define EINA_PLAYER_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.player"
#define EINA_PLAYER_STREAM_MARKUP_KEY  "stream-markup"

typedef struct _EinaPlayer EinaPlayer;

#define eina_plugin_player_get_cover_widget(plugin) eina_player_get_cover_widget(eina_plugin_get_player(plugin))
EinaCover *
eina_player_get_cover_widget(EinaPlayer* self);

G_END_DECLS

#endif // _PLAYER_H
