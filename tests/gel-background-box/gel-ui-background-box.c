/*
 * tests/gel-background-box/gel-ui-background-box.c
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
*            cd-type-chooser.c
*
*  ven mai 27 17:33:12 2005
*  Copyright  2005  Philippe Rouquier
*  <brasero-app@wanadoo.fr>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>

#include <gtk/gtk.h>
#include "gel-ui-background-box.h"

G_DEFINE_TYPE (GelUIBackgroundBox, gel_ui_background_box, GTK_TYPE_EVENT_BOX);


struct GelUIBackgroundBoxPrivate {
	GObject *background;
};

static GObjectClass *parent_class = NULL;

static void
gel_ui_background_box_init (GelUIBackgroundBox *obj)
{
	obj->priv = g_new0 (GelUIBackgroundBoxPrivate, 1);
	obj->priv->background;
}

/* Cut and Pasted from Gtk+ gtkeventbox.c but modified to display back image */
static gboolean
gel_ui_background_box_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GelUIBackgroundBox *chooser = GEL_UI_BACKGROUND_BOX(widget);

	if (GTK_WIDGET_DRAWABLE (widget))
	{
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
		if (!GTK_WIDGET_NO_WINDOW(widget) && !GTK_WIDGET_APP_PAINTABLE (widget) && chooser->priv->background)
		{
			if (G_OBJECT_TYPE(chooser->priv->background) == GDK_TYPE_PIXBUF)
				gdk_draw_pixbuf (widget->window, widget->style->white_gc,
					(GdkPixbuf*) chooser->priv->background,
					 0, 0,
					 0, 0,
					-1, -1,
					GDK_RGB_DITHER_NORMAL,
					 0, 0);
			else if (GDK_IS_DRAWABLE(chooser->priv->background))
				gdk_draw_drawable(widget->window, widget->style->white_gc,
					(GdkDrawable *) chooser->priv->background,
					0, 0,
					0, 0,
					-1, -1);
		}
	}

	g_printf("CAlled\n");
	
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
	GList *l = children;
	while (l)
	{
		gtk_container_propagate_expose(GTK_CONTAINER(widget), GTK_WIDGET(l->data), event);
		l = l->next;
	}
	g_list_free(children);


	return TRUE;
}

static void
gel_ui_background_box_finalize (GObject *object)
{
	GelUIBackgroundBox *cobj;

	cobj = GEL_UI_BACKGROUND_BOX (object);

	if (cobj->priv->background) {
		g_object_unref (G_OBJECT (cobj->priv->background));
		cobj->priv->background = NULL;
	}

	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gel_ui_background_box_class_init (GelUIBackgroundBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = gel_ui_background_box_finalize;
	widget_class->expose_event = gel_ui_background_box_expose_event;
}

GtkWidget *
gel_ui_background_box_new (GtkWidget *w)
{
	GelUIBackgroundBox *obj;

	obj = GEL_UI_BACKGROUND_BOX(g_object_new (GEL_UI_TYPE_BACKGROUND_BOX,
					    NULL));

	return GTK_WIDGET (obj);
}

void
gel_ui_background_set_pixbuf(GelUIBackgroundBox *self, GdkPixbuf *pixbuf)
{
	GdkEvent *ev = gdk_event_new(GDK_EXPOSE);
	self->priv->background = G_OBJECT(pixbuf);
	gel_ui_background_box_expose_event(GTK_WIDGET(self), (GdkEventExpose*) ev);
	gdk_event_free(ev);
}

void
gel_ui_background_set_drawable(GelUIBackgroundBox *self, GdkDrawable *drawable)
{
	GdkEvent *ev = gdk_event_new(GDK_EXPOSE);
	self->priv->background = G_OBJECT(drawable);
	gel_ui_background_box_expose_event(GTK_WIDGET(self), (GdkEventExpose*) ev);
	gdk_event_free(ev);
}

