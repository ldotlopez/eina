/*
 * gel/gel-bind.h
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

#ifndef _GEL_BIND_H_
#define _GEL_BIND_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GelObjectBind GelObjectBind;
typedef gboolean (*GelObjectBindMappingFunc)(GValue *a, GValue *b);

#define gel_object_bind(src,s_prop,dst,d_prop) gel_object_bind_with_mapping(src,s_prop,dst,d_prop,NULL)
#define gel_object_bind_mutual(src,s_prop,dst,d_prop) \
	G_STMT_START { \
		gel_object_bind_with_mapping(src,s_prop,dst,d_prop,NULL); \
		gel_object_bind_with_mapping(dst,d_prop,src,s_prop,NULL); \
	} G_STMT_END
#define gel_object_bind_mutual_with_mapping(src,s_prop,dst,d_prop,mapping) \
	G_STMT_START { \
		gel_object_bind_with_mapping(src,s_prop,dst,d_prop,mapping); \
		gel_object_bind_with_mapping(dst,d_prop,src,s_prop,mapping); \
	} G_STMT_END

GelObjectBind*
gel_object_bind_with_mapping(GObject *src, gchar *s_prop, GObject *dst, gchar *d_prop, GelObjectBindMappingFunc mapping);

void
gel_object_unbind(GelObjectBind *bind);

G_END_DECLS

#endif // _GEL_BIND_H

