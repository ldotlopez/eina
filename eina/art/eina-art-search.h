/*
 * eina/art/eina-art-search.h
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

#ifndef _EINA_ART_SEARCH
#define _EINA_ART_SEARCH

#include <glib-object.h>
#include <lomo/lomo-player.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_SEARCH eina_art_search_get_type()

#define EINA_ART_SEARCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_SEARCH, EinaArtSearch))

#define EINA_ART_SEARCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART_SEARCH, EinaArtSearchClass))

#define EINA_IS_ART_SEARCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_SEARCH))

#define EINA_IS_ART_SEARCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART_SEARCH))

#define EINA_ART_SEARCH_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART_SEARCH, EinaArtSearchClass))

typedef struct {
	GObject parent;
} EinaArtSearch;

typedef struct {
	GObjectClass parent_class;
} EinaArtSearchClass;

GType eina_art_search_get_type (void);

typedef void (*EinaArtSearchCallback) (EinaArtSearch *search, gpointer data);

EinaArtSearch* eina_art_search_new (GObject *domain,
                                    LomoStream *stream,
                                    EinaArtSearchCallback callback,
                                    gpointer callback_data);

const gchar *eina_art_search_stringify(EinaArtSearch *search);

LomoStream  *eina_art_search_get_stream(EinaArtSearch *search);
GObject     *eina_art_search_get_domain(EinaArtSearch *search);

void     eina_art_search_set_bpointer(EinaArtSearch *search, gpointer bpointer);
gpointer eina_art_search_get_bpointer(EinaArtSearch *search);

void         eina_art_search_set_result(EinaArtSearch *search, const gchar *uri);
const gchar *eina_art_search_get_result(EinaArtSearch *search);
GdkPixbuf   *eina_art_search_get_result_as_pixbuf(EinaArtSearch *search);

void
eina_art_search_run_callback(EinaArtSearch *search);



G_END_DECLS

#endif /* _EINA_ART_SEARCH */
