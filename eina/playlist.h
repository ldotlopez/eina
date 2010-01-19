/*
 * eina/playlist.h
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

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_PLAYLIST(p)           ((EinaPlaylist *) p)
#define GEL_APP_GET_PLAYLIST(app)  EINA_PLAYLIST(gel_app_shared_get(app,"playlist"))
#define EINA_OBJ_GET_PLAYLIST(obj) GEL_HUB_GET_PLAYLIST(eina_obj_get_app(obj))

typedef struct _EinaPlaylist EinaPlaylist;

G_END_DECLS

#endif // _PLAYLIST_H
