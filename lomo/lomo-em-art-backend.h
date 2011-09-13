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

#ifndef _LOMO_EM_ART_BACKEND
#define _LOMO_EM_ART_BACKEND

#include <glib-object.h>
#include <lomo/lomo-em-art-search.h>

G_BEGIN_DECLS

#define EINA_TYPE_ART_BACKEND lomo_em_art_backend_get_type()

#define LOMO_EM_ART_BACKEND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_ART_BACKEND, LomoEMArtBackend))
#define LOMO_EM_ART_BACKEND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_ART_BACKEND, LomoEMArtBackendClass))
#define LOMO_IS_EM_ART_BACKEND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_ART_BACKEND))
#define LOMO_IS_EM_ART_BACKEND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_ART_BACKEND))
#define LOMO_EM_ART_BACKEND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_ART_BACKEND, LomoEMArtBackendClass))

typedef struct {
	/* <private> */
	GObject parent;
} LomoEMArtBackend;

typedef struct {
	GObjectClass parent_class;
	void (*finish) (LomoEMArtBackend *backend, LomoEMArtSearch *search);
} LomoEMArtBackendClass;

typedef void (*LomoEMArtBackendFunc) (LomoEMArtBackend *backend, LomoEMArtSearch *search, gpointer backend_data);

GType lomo_em_art_backend_get_type (void);

LomoEMArtBackend* lomo_em_art_backend_new (const gchar *name,
	LomoEMArtBackendFunc search, LomoEMArtBackendFunc cancel, GDestroyNotify notify, gpointer backend_data);
const gchar *
lomo_em_art_backend_get_name(LomoEMArtBackend *backend);

void
lomo_em_art_backend_run(LomoEMArtBackend *backend, LomoEMArtSearch *search);
void
lomo_em_art_backend_cancel(LomoEMArtBackend *backend, LomoEMArtSearch *search);
void
lomo_em_art_backend_finish(LomoEMArtBackend *backend, LomoEMArtSearch *search);

G_END_DECLS

#endif /* _LOMO_EM_ART_BACKEND */
