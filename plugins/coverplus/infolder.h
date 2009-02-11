/*
 * plugins/coverplus/infolder.h
 *
 * Copyright (C) 2004-2009 Eina
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

#ifndef _EINA_PLUGIN_COVERPLUS_INFOLDER_H
#define _EINA_PLUGIN_COVERPLUS_INFOLDER_H

#include <eina/eina-plugin.h>

G_BEGIN_DECLS

typedef struct _CoverPlusInfolder CoverPlusInfolder;

CoverPlusInfolder*
coverplus_infolder_new(EinaPlugin *plugin, GError **error);
void
coverplus_infolder_destroy(CoverPlusInfolder *self);

void
coverplus_infolder_art_search_cb(Art *art, ArtSearch *search, CoverPlusInfolder *self);
void
coverplus_infolder_search_cb(EinaArtwork *artwork, LomoStream *stream, CoverPlusInfolder *self);
void
coverplus_infolder_cancel_cb(EinaArtwork *artwork, CoverPlusInfolder *self);

G_END_DECLS

#endif
