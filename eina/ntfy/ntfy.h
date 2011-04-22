/*
 * eina/ntfy/ntfy.h
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
#ifndef _EINA_PLUGIN_NTFY_H
#define _EINA_PLUGIN_NTFY_H

#include <config.h>
#include <glib.h>

#define EINA_NTFY_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.ntfy"
#define EINA_NTFY_ENABLED_KEY        "enabled"

typedef struct _EinaNtfy EinaNtfy;

enum {
	EINA_NTFY_NO_ERROR = 0,
	EINA_NTFY_LIBRARY_ERROR,
	EINA_NTFY_SETTINGS_ERROR
} EinaNtfyError;

#endif

