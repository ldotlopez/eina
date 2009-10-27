/*
 * eina/eina-stock.c
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

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <eina/eina-stock.h>

static gboolean done = FALSE;

static const  struct {
	gchar *stock_id;
	gchar *icon;
} stock_icons [] = {
	#if defined(__APPLE__) || defined(__APPLE_CC__)
	{ EINA_STOCK_STATUS_ICON, "osx-status-icon"      },
	#else
	{ EINA_STOCK_STATUS_ICON, "standard-status-icon" },
	#endif
	{ EINA_STOCK_RANDOM,      "random"      },
	{ EINA_STOCK_REPEAT,      "repeat"      },
	{ EINA_STOCK_QUEUE ,      "queue"       },
	{ EINA_STOCK_BUG,         "bug"         },
	{ EINA_STOCK_PLUGIN,      "plugin"      }
};

static const GtkStockItem stock_items [] = {
	{ EINA_STOCK_STATUS_ICON, N_("Status icon"),  0, 0, GETTEXT_PACKAGE },
	{ EINA_STOCK_RANDOM,      N_("_Random"),      0, 0, GETTEXT_PACKAGE },
	{ EINA_STOCK_REPEAT,      N_("_Shuffle"),     0, 0, GETTEXT_PACKAGE },
	{ EINA_STOCK_QUEUE,       N_("_Queue"),       0, 0, GETTEXT_PACKAGE },
	{ EINA_STOCK_BUG,         N_("Report a bug"), 0, 0, GETTEXT_PACKAGE },
	{ EINA_STOCK_PLUGIN,      NULL,               0, 0, GETTEXT_PACKAGE }
};

void
eina_stock_init(void)
{        
	GtkIconFactory *factory;
	GtkIconSource  *source;
	gint             i;
                    	
	if (done)
		return;
	done = TRUE;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));

	factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (factory);

	source = gtk_icon_source_new ();

	for (i = 0; i < G_N_ELEMENTS (stock_icons); i++)
	{
		GtkIconSet *set;

		gtk_icon_source_set_icon_name (source, stock_icons [i].icon);

		set = gtk_icon_set_new ();
		gtk_icon_set_add_source (set, source);

		gtk_icon_factory_add (factory, stock_icons [i].stock_id, set);
		gtk_icon_set_unref (set);
	}

	gtk_icon_source_free (source);

	g_object_unref (factory);
}

