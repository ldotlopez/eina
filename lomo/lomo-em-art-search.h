/*
 * lomo/lomo-em-art-search.h
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

#ifndef _LOMO_EM_ART_SEARCH
#define _LOMO_EM_ART_SEARCH

#include <glib-object.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_SEARCH lomo_em_art_search_get_type()

#define LOMO_EM_ART_SEARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_SEARCH, LomoEMArtSearch))
#define LOMO_EM_ART_SEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_ART_SEARCH, LomoEMArtSearchClass))
#define LOMO_IS_EM_ART_SEARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_SEARCH))
#define LOMO_IS_EM_ART_SEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_ART_SEARCH))
#define LOMO_EM_ART_SEARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_ART_SEARCH, LomoEMArtSearchClass))

typedef struct {
	GObject parent;
} LomoEMArtSearch;

typedef struct {
	GObjectClass parent_class;
} LomoEMArtSearchClass;

GType lomo_em_art_search_get_type (void);

typedef void (*LomoEMArtSearchCallback) (LomoEMArtSearch *search, gpointer data);

LomoEMArtSearch* lomo_em_art_search_new (GObject *domain,
                                    LomoStream *stream,
                                    LomoEMArtSearchCallback callback,
                                    gpointer callback_data);

const gchar *lomo_em_art_search_stringify(LomoEMArtSearch *search);

LomoStream  *lomo_em_art_search_get_stream(LomoEMArtSearch *search);
GObject     *lomo_em_art_search_get_domain(LomoEMArtSearch *search);

void     lomo_em_art_search_set_bpointer(LomoEMArtSearch *search, gpointer bpointer);
gpointer lomo_em_art_search_get_bpointer(LomoEMArtSearch *search);

void         lomo_em_art_search_set_result(LomoEMArtSearch *search, const gchar *uri);
const gchar *lomo_em_art_search_get_result(LomoEMArtSearch *search);

void
lomo_em_art_search_run_callback(LomoEMArtSearch *search);

G_END_DECLS

#endif /* _LOMO_EM_ART_SEARCH */
