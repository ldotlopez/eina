/*
 * eina/ntfy/eina-ntfy-plugin.h
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

#ifndef __EINA_NTFY_PLUGIN_H__
#define __EINA_NTFY_PLUGIN_H__

#include <eina/ext/eina-extension.h>

G_BEGIN_DECLS

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_NTFY_PLUGIN         (eina_ntfy_plugin_get_type ())
#define EINA_NTFY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_NTFY_PLUGIN, EinaNtfyPlugin))
#define EINA_NTFY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_NTFY_PLUGIN, EinaNtfyPlugin))
#define EINA_IS_NTFY_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_NTFY_PLUGIN))
#define EINA_IS_NTFY_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_NTFY_PLUGIN))
#define EINA_NTFY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_NTFY_PLUGIN, EinaNtfyPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaNtfyPlugin, eina_ntfy_plugin)

/**
 * API
 */
#define EINA_NTFY_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.ntfy"
#define EINA_NTFY_ENABLED_KEY        "enabled"

/**
 * EinaNtfyPluginError:
 * @EINA_NTFY_PLUGIN_ERROR_LIBRARY: Libray error
 * @EINA_NTFY_PLUGIN_ERROR_SETTNGS: Settings error
 */
typedef enum {
	EINA_NTFY_PLUGIN_ERROR_LIBRARY = 1,
	EINA_NTFY_PLUGIN_ERROR_SETTNGS
} EinaNtfyError;

G_END_DECLS

#endif // __EINA_NTFY_PLUGIN_H__

