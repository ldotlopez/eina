/*
 * eina/art/eina-art.h
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

#ifndef _EINA_ART
#define _EINA_ART

#include <glib-object.h>
#include <eina/art/eina-art-backend.h>
#include <eina/art/eina-art-search.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART eina_art_get_type()

#define EINA_ART(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART, EinaArt))

#define EINA_ART_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART, EinaArtClass))

#define EINA_IS_ART(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART))

#define EINA_IS_ART_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART))

#define EINA_ART_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART, EinaArtClass))

typedef struct {
	GObject parent;
} EinaArt;

typedef struct _EinaArtClassPrivate EinaArtClassPrivate;
typedef struct {
	GObjectClass parent_class;
	EinaArtClassPrivate *priv;
} EinaArtClass;

EinaArtBackend* eina_art_class_add_backend(EinaArtClass *art_class,
                           gchar *name,
                           EinaArtBackendFunc search, EinaArtBackendFunc cancel,
                           GDestroyNotify notify, gpointer data);

void eina_art_class_remove_backend(EinaArtClass *art_class, EinaArtBackend *backend);

GType eina_art_get_type (void);

EinaArt* eina_art_new (void);

EinaArtSearch*
eina_art_search(EinaArt *art, LomoStream *stream, EinaArtSearchCallback callback, gpointer data);

G_END_DECLS

#endif /* _EINA_ART */
