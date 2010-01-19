/*
 * gel/gel-string.c
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

#include "gel-string.h"

struct _GelStringPriv {
	guint ref_count;
};

GelString *
gel_string_new(gchar *str)
{
	GelString *self = g_new0(GelString, 1);
	self->priv = g_new0(GelStringPriv,  1);

	if (str != NULL)
		self->str = g_strdup(str);
	self->priv->ref_count = 1;

	return self;
}

void
gel_string_ref(GelString *self)
{
	self->priv->ref_count++;
}

void
gel_string_unref(GelString *self)
{
	self->priv->ref_count--;

	if (self->priv->ref_count > 0)
		return;

	g_free(self->priv);
	g_free(self->str);
	g_free(self);
}

