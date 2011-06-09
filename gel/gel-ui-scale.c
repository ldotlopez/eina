/*
 * gel/gel-ui-scale.c
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
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "gel-ui-scale.h"

G_DEFINE_TYPE (GelUIScale, gel_ui_scale, GTK_TYPE_SCALE)

static void
gel_ui_scale_class_init (GelUIScaleClass *klass)
{
}

static void
sync_value(GelUIScale *self)
{
	gtk_range_set_fill_level (GTK_RANGE (self), gtk_range_get_value (GTK_RANGE (self)));
}

static void
gel_ui_scale_init (GelUIScale *self)
{
	gtk_range_set_show_fill_level (GTK_RANGE (self), TRUE);
	gtk_range_set_restrict_to_fill_level (GTK_RANGE (self), FALSE);

	sync_value(self);
	g_signal_connect (self, "value-changed", (GCallback) sync_value, NULL);
}

GelUIScale*
gel_ui_scale_new (void)
{
	return g_object_new (GEL_UI_TYPE_SCALE, NULL);
}
