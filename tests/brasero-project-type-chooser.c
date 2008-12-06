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
// #include <glib/gi18n-lib.h>
#include <glib-object.h>

#include <gtk/gtkwidget.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktable.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkmisc.h>
#include "brasero-project-type-chooser.h"

G_DEFINE_TYPE (BraseroProjectTypeChooser, brasero_project_type_chooser, GTK_TYPE_EVENT_BOX);


struct BraseroProjectTypeChooserPrivate {
	GdkPixbuf *background;
};

static GObjectClass *parent_class = NULL;

static void
brasero_project_type_chooser_init (BraseroProjectTypeChooser *obj)
{
	GError *error = NULL;
	obj->priv = g_new0 (BraseroProjectTypeChooserPrivate, 1);
	obj->priv->background = gdk_pixbuf_new_from_file ("logo.png", &error);
	if (error) {
		g_warning ("ERROR loading background pix : %s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
}

/* Cut and Pasted from Gtk+ gtkeventbox.c but modified to display back image */
static gboolean
brasero_project_type_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	BraseroProjectTypeChooser *chooser;

	chooser = BRASERO_PROJECT_TYPE_CHOOSER (widget);

	if (GTK_WIDGET_DRAWABLE (widget))
	{
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

		if (!GTK_WIDGET_NO_WINDOW (widget)) {
			if (!GTK_WIDGET_APP_PAINTABLE (widget)
			&&  chooser->priv->background) {
				int width, offset = 150;

				width = gdk_pixbuf_get_width (chooser->priv->background);
				gdk_draw_pixbuf (widget->window,
					         widget->style->white_gc,
						 chooser->priv->background,
						 offset,
						 0,
						 0,
						 0,
						 width - offset,
						 -1,
						 GDK_RGB_DITHER_NORMAL,
						 0, 0);
			}
		}
	}
    GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
	    GList *l = children;
		    while (l)
			    {
				        gtk_container_propagate_expose(GTK_CONTAINER(widget), GTK_WIDGET(l->data), event);
						        l = l->next;
								    }



	return FALSE;
}

static void
brasero_project_type_chooser_finalize (GObject *object)
{
	BraseroProjectTypeChooser *cobj;

	cobj = BRASERO_PROJECT_TYPE_CHOOSER (object);

	if (cobj->priv->background) {
		g_object_unref (G_OBJECT (cobj->priv->background));
		cobj->priv->background = NULL;
	}

	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
brasero_project_type_chooser_class_init (BraseroProjectTypeChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = brasero_project_type_chooser_finalize;
	widget_class->expose_event = brasero_project_type_expose_event;
}

GtkWidget *
brasero_project_type_chooser_new (GtkWidget *w)
{
	BraseroProjectTypeChooser *obj;

	obj = BRASERO_PROJECT_TYPE_CHOOSER (g_object_new (BRASERO_TYPE_PROJECT_TYPE_CHOOSER,
					    NULL));
	    gtk_container_add(GTK_CONTAINER(obj), w);


	return GTK_WIDGET (obj);
}
