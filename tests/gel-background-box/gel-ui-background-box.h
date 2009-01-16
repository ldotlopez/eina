/*
 * tests/gel-background-box/gel-ui-background-box.h
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

/***************************************************************************
*            cd-type-chooser.h
*
*  ven mai 27 17:33:12 2005
*  Copyright  2005  Philippe Rouquier
*  brasero-app@wanadoo.fr
****************************************************************************/

/*
 *  Brasero is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Brasero is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _GEL_UI_BACKGROUND_BOX_H
#define _GEL_UI_BACKGROUND_BOX_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEL_UI_TYPE_BACKGROUND_BOX         (gel_ui_background_box_get_type ())
#define GEL_UI_BACKGROUND_BOX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GEL_UI_TYPE_BACKGROUND_BOX, GelUIBackgroundBox))
#define GEL_UI_BACKGROUND_BOX_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),GEL_UI_TYPE_BACKGROUND_BOX, GelUIBackgroundBoxClass))
#define GEL_UI_IS_BACKGROUND_BOX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GEL_UI_TYPE_BACKGROUND_BOX))
#define GEL_UI_IS_BACKGROUND_BOX_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GEL_UI_TYPE_BACKGROUND_BOX))
#define GEL_UI_BACKGROUND_BOX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GEL_UI_TYPE_BACKGROUND_BOX, GelUIBackgroundBoxClass))

typedef struct GelUIBackgroundBoxPrivate GelUIBackgroundBoxPrivate;

typedef struct {
	GtkEventBox parent;
	GelUIBackgroundBoxPrivate *priv;
} GelUIBackgroundBox;

typedef struct {
	GtkEventBoxClass parent_class;
} GelUIBackgroundBoxClass;

GType gel_ui_background_box_get_type ();
GtkWidget *gel_ui_background_box_new ();

void
gel_ui_background_set_pixbuf(GelUIBackgroundBox *self, GdkPixbuf *pixbuf);
void
gel_ui_background_set_drawable(GelUIBackgroundBox *self, GdkDrawable *drawable);

#endif // _GEL_UI_BACKGROUND_BOX_H
