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

#include <eina/core/eina-extension.h>

G_BEGIN_DECLS

/**
 * API
 */
#define EINA_NTFY_PREFERENCES_DOMAIN EINA_APP_DOMAIN".preferences.ntfy"
#define EINA_NTFY_ENABLED_KEY        "enabled"

/**
 * EinaNtfyPluginError:
 * @EINA_NTFY_PLUGIN_ERROR_LIBRARY: Libray error
 * @EINA_NTFY_PLUGIN_ERROR_SETTNGS: Settings error
 */
typedef enum {
	EINA_NTFY_PLUGIN_ERROR_LIBRARY = 1,
	EINA_NTFY_PLUGIN_ERROR_SETTNGS
} EinaNtfyPluginError;

G_END_DECLS

#endif // __EINA_NTFY_PLUGIN_H__

