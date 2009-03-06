/*
 * lomo/meta.h
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

#ifndef _LOMO_META
#define _LOMO_META

#include <glib-object.h>
#include <gst/gst.h>
#include "player.h"
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_TYPE_META lomo_meta_get_type()

#define LOMO_META(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_META, LomoMeta))

#define LOMO_META_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), LOMO_TYPE_META, LomoMetaClass))

#define LOMO_IS_META(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_META))

#define LOMO_IS_META_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), LOMO_TYPE_META))

#define LOMO_META_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), LOMO_TYPE_META, LomoMetaClass))

typedef struct {
	GObject parent;
} LomoMeta;

typedef struct {
	GObjectClass parent_class;
} LomoMetaClass;

typedef enum {
	LOMO_META_PRIO_INMEDIATE,
	LOMO_META_PRIO_DEFAULT
} LomoMetaPrio;

GType lomo_meta_get_type (void);

LomoMeta* lomo_meta_new (LomoPlayer *player);
void lomo_meta_parse(LomoMeta *meta, LomoStream *stream, LomoMetaPrio prio);
void lomo_meta_clear(LomoMeta *meta);

G_END_DECLS

#endif /* _LOMO_META */
