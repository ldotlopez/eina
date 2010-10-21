/*
 * eina/art/art.h
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

#ifndef _ART
#define _ART

#include <eina/art/eina-art.h>

G_BEGIN_DECLS

#define gel_engine_plugin_get_art(engine) eina_art_new()
#define eina_plugin_get_art(plugin)       eina_art_new()

G_END_DECLS

#endif

