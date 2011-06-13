/*
 * eina/player/player.h
 *
 * Copyright (C) 2004-2011 Eina
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

#include <eina/ext/eina-extension.h>
#include <eina/player/eina-player.h>

#define EINA_TYPE_PLAYER_PLUGIN         (eina_player_plugin_get_type ())
#define EINA_PLAYER_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPlugin))
#define EINA_PLAYER_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPlugin))
#define EINA_IS_PLAYER_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_PLAYER_PLUGIN))
#define EINA_IS_PLAYER_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_PLAYER_PLUGIN))
#define EINA_PLAYER_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_PLAYER_PLUGIN, EinaPlayerPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaPlayerPlugin, eina_player_plugin)

#define eina_application_get_player(app) eina_application_get_interface(app, "player")
#define eina_plugin_get_player(plugin)   eina_application_get_player(eina_plugin_get_application(plugin))

#endif

