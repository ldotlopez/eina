/*
 * eina/player/eina-player-plugin.h
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

#ifndef __EINA_PLAYER_PLUGIN_H__
#define __EINA_PLAYER_PLUGIN_H__

#include <eina/ext/eina-application.h>
#include <eina/player/eina-player.h>

G_BEGIN_DECLS

/**
 * EinaApplication accessors
 */
EinaPlayer *eina_application_get_player(EinaApplication *application);

G_END_DECLS

#endif // __EINA_PLAYER_PLUGIN_H__

