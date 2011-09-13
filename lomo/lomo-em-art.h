/*
 * lomo/lomo-em-art.h
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

#ifndef __LOMO_EM_ART_H__
#define __LOMO_EM_ART_H__

#include <glib-object.h>
#include <lomo/lomo-em-art-backend.h>
#include <lomo/lomo-em-art-search.h>

G_BEGIN_DECLS

#define LOMO_TYPE_EM_ART lomo_em_art_get_type()

#define LOMO_EM_ART(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_EM_ART, LomoEMArt))
#define LOMO_EM_ART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LOMO_TYPE_EM_ART, LomoEMArtClass))
#define LOMO_IS_EM_ART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_EM_ART))
#define LOMO_IS_EM_ART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LOMO_TYPE_EM_ART))
#define LOMO_EM_ART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LOMO_TYPE_EM_ART, LomoEMArtClass))

typedef struct {
	/* <private> */
	GObject parent;
} LomoEMArt;

typedef struct _LomoEMArtClassPrivate LomoEMArtClassPrivate;
typedef struct {
	/* <private> */
	GObjectClass parent_class;
	LomoEMArtClassPrivate *priv;
} LomoEMArtClass;

LomoEMArtBackend* lomo_em_art_class_add_backend(LomoEMArtClass *art_class,
	gchar *name,
	LomoEMArtBackendFunc search, LomoEMArtBackendFunc cancel,
	GDestroyNotify notify, gpointer data);

void lomo_em_art_class_remove_backend(LomoEMArtClass *art_class, LomoEMArtBackend *backend);

GType lomo_em_art_get_type (void);

LomoEMArt* lomo_em_art_new (void);

LomoEMArtSearch*
lomo_em_art_search(LomoEMArt *art, LomoStream *stream, LomoEMArtSearchCallback callback, gpointer data);
void
lomo_em_art_cancel(LomoEMArt *art, LomoEMArtSearch *search);

G_END_DECLS

#endif /* __LOMO_EM_ART_H__ */
