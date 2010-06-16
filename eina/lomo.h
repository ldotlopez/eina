/*
 * eina/lomo.h
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

#ifndef _EINA_LOMO_H
#define _EINA_LOMO_H

#include <gel/gel.h>
#include <lomo/lomo-player.h>
#include <eina/eina-plugin.h>

G_BEGIN_DECLS

#define EINA_LOMO_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.lomo"
#define EINA_LOMO_VOLUME_KEY     "volume"
#define EINA_LOMO_MUTE_KEY       "mute"
#define EINA_LOMO_REPEAT_KEY     "repeat"
#define EINA_LOMO_RANDOM_KEY     "random"
#define EINA_LOMO_AUTO_PARSE_KEY "auto-parse"
#define EINA_LOMO_AUTO_PLAY_KEY  "auto-play"
#define EINA_LOMO_CURRENT_KEY    "current-stream"

// FIXME:
#define EinaPlugin GelPlugin

typedef enum { 
	EINA_LOMO_NO_ERROR = 0, 
	EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE, 
	EINA_LOMO_ERROR_CANNOT_SET_SHARED, 
	EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE 
} EinaLomoError; 

#define gel_app_get_lomo(app)        LOMO_PLAYER(gel_app_shared_get(app,"lomo"))
#define eina_plugin_get_lomo(plugin) gel_app_get_lomo(gel_plugin_get_app(plugin))
#define eina_obj_get_lomo(obj)       gel_app_get_lomo(eina_obj_get_app(obj))

gpointer
eina_plugin_lomo_add_handlers(EinaPlugin *plugin, ...);

void
eina_plugin_lomo_remove_handlers(EinaPlugin *plugin, gpointer handler_pointer); 

G_END_DECLS

#endif // _EINA_LOMO_H

