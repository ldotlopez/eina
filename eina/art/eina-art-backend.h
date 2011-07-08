/*
 * eina/art/eina-art-backend.h
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

#ifndef _EINA_ART_BACKEND
#define _EINA_ART_BACKEND

#include <glib-object.h>
#include <eina/art/eina-art-search.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_BACKEND eina_art_backend_get_type()

#define EINA_ART_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_BACKEND, EinaArtBackend))

#define EINA_ART_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_ART_BACKEND, EinaArtBackendClass))

#define EINA_IS_ART_BACKEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_BACKEND))

#define EINA_IS_ART_BACKEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_ART_BACKEND))

#define EINA_ART_BACKEND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_ART_BACKEND, EinaArtBackendClass))

typedef struct {
	GObject parent;
} EinaArtBackend;

typedef struct {
	GObjectClass parent_class;
	void (*finish) (EinaArtBackend *backend, EinaArtSearch *search);
} EinaArtBackendClass;

typedef void (*EinaArtBackendFunc) (EinaArtBackend *backend, EinaArtSearch *search, gpointer backend_data);

GType eina_art_backend_get_type (void);

EinaArtBackend* eina_art_backend_new (gchar *name,
	EinaArtBackendFunc search, EinaArtBackendFunc cancel, GDestroyNotify notify, gpointer backend_data);
const gchar *
eina_art_backend_get_name(EinaArtBackend *backend);

void
eina_art_backend_run(EinaArtBackend *backend, EinaArtSearch *search);
void
eina_art_backend_cancel(EinaArtBackend *backend, EinaArtSearch *search);
void
eina_art_backend_finish(EinaArtBackend *backend, EinaArtSearch *search);

G_END_DECLS

#endif /* _EINA_ART_BACKEND */
