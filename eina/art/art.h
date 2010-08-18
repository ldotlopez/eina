/*
 * eina/art.h
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

#ifndef _ART_H
#define _ART_H

#include <glib.h>
#include <gdk/gdk.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define gel_plugin_engine_get_art(engine) gel_plugin_engine_get_interface(engine, "art")

typedef struct _Art        Art;
typedef struct _ArtBackend ArtBackend;
typedef struct _ArtSearch  ArtSearch;

typedef void (*ArtFunc)(Art *art, ArtSearch *search, gpointer data);
#define ART_FUNC(x) ((ArtFunc) x)

Art* art_new(void);
void art_destroy(Art *art);

ArtBackend* art_add_backend(Art *art, gchar *name, ArtFunc search_func, ArtFunc cancel_func, gpointer data);
void        art_remove_backend(Art *art, ArtBackend *backend);

const gchar *art_backend_get_name(ArtBackend *backend);

ArtSearch* art_search(Art *art, LomoStream *stream, ArtFunc callback, gpointer pointer);
void       art_cancel(Art *art, ArtSearch *search);

void art_report_success(Art *art, ArtSearch *search, GdkPixbuf *result);
void art_report_failure(Art *art, ArtSearch *search);

LomoStream *art_search_get_stream(ArtSearch *search);
gpointer    art_search_get_result(ArtSearch *search);
gpointer    art_search_get_data  (ArtSearch *search);

G_END_DECLS

#endif
