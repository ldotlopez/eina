/*
 * gel/gel-ui-scale.h
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

#ifndef _GEL_UI_SCALE
#define _GEL_UI_SCALE

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEL_UI_TYPE_SCALE gel_ui_scale_get_type()

#define GEL_UI_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_UI_TYPE_SCALE, GelUIScale))
#define GEL_UI_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEL_UI_TYPE_SCALE, GelUIScaleClass))
#define GEL_UI_IS_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_UI_TYPE_SCALE))
#define GEL_UI_IS_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEL_UI_TYPE_SCALE))
#define GEL_UI_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEL_UI_TYPE_SCALE, GelUIScaleClass))

typedef struct {
	GtkScale parent;
} GelUIScale;

typedef struct {
	GtkScaleClass parent_class;
} GelUIScaleClass;

GType gel_ui_scale_get_type (void);

GelUIScale* gel_ui_scale_new (void);

G_END_DECLS

#endif /* _GEL_UI_SCALE */

