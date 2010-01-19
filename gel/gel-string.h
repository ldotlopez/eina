/*
 * gel/gel-string.h
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

#ifndef _GEL_STRING
#define _GEL_STRING

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GelString GelString;
typedef struct _GelStringPriv GelStringPriv;

struct _GelString {
	gchar *str;
	GelStringPriv *priv;
};

GelString *gel_string_new(gchar *str);

void gel_string_ref  (GelString *self);
void gel_string_unref(GelString *self);

G_END_DECLS

#endif // _GEL_STRING
