/*
 * eina/lomo/eina-lomo-plugin.h
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

#ifndef __EINA_LOMO_PLUGIN_H__
#define __EINA_LOMO_PLUGIN_H__

#include <eina/ext/eina-extension.h>
#include <lomo/lomo-player.h>
#include <lomo/lomo-util.h>

G_BEGIN_DECLS

#define EINA_TYPE_LOMO_PLUGIN         (eina_lomo_plugin_get_type ())
#define EINA_LOMO_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_LOMO_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_IS_LOMO_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_LOMO_PLUGIN))
#define EINA_IS_LOMO_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_LOMO_PLUGIN))
#define EINA_LOMO_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_LOMO_PLUGIN, EinaLomoPluginClass))

#define EINA_LOMO_PREFERENCES_DOMAIN  EINA_DOMAIN".preferences.lomo"
#define EINA_LOMO_VOLUME_KEY          "volume"
#define EINA_LOMO_MUTE_KEY            "mute"
#define EINA_LOMO_REPEAT_KEY          "repeat"
#define EINA_LOMO_RANDOM_KEY          "random"
#define EINA_LOMO_AUTO_PARSE_KEY      "auto-parse"
#define EINA_LOMO_AUTO_PLAY_KEY       "auto-play"
// #define EINA_LOMO_CURRENT_KEY    "current-stream"
// #define EINA_LOMO_CURRENT_STREAM_KEY "current-stream"

EINA_DEFINE_EXTENSION_HEADERS(EinaLomoPlugin, eina_lomo_plugin)

/**
 * EinaLomoError:
 * @EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE: LomoPlayer engine cannot be created
 * @EINA_LOMO_ERROR_CANNOT_SET_SHARED: LomoPlayer engine cannot be inserted
 *                                     into EinaApplication
 * @EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE: LomoPlayer engine cannot be
 *                                         destroyed
 */
typedef enum { 
	EINA_LOMO_ERROR_CANNOT_CREATE_ENGINE = 1,
	EINA_LOMO_ERROR_CANNOT_SET_SHARED, 
	EINA_LOMO_ERROR_CANNOT_DESTROY_ENGINE 
} EinaLomoError; 

#define eina_application_get_lomo(app)  eina_application_get_interface(app,"lomo")
#define eina_plugin_get_lomo(plugin)    eina_application_get_lomo(eina_plugin_get_application(plugin))

G_END_DECLS

#endif /* __EINA_LOMO_PLUGIN_H__ */
