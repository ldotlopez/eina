/*
 * eina/dock.c
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

#define GEL_DOMAIN "Eina::Dock"

#include <config.h>
#include <string.h>
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/dock.h>
#include <eina/eina-plugin.h>
#include <eina/window.h>

struct _EinaDock {
	EinaObj      parent;

	GtkWidget   *widget;
	GtkNotebook *dock;
	GHashTable  *dock_items;
	gchar     **dock_idx;
	
	gint         w, h;
};

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);

static void
expander_activate_cb(GtkExpander *wi, EinaDock *self);
static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self);

G_MODULE_EXPORT gboolean
dock_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaDock *self;

	self = g_new0(EinaDock, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "dock", EINA_OBJ_GTK_UI, error))
		return FALSE;

	self->widget = eina_obj_get_widget(self, "dock-expander");
	self->dock   = eina_obj_get_typed(self, GTK_NOTEBOOK, "dock-notebook");

	// Setup things
	GSettings *dock_settings = gel_app_get_gsettings(app, EINA_DOMAIN".preferences.dock");
	gboolean expanded = g_settings_get_boolean(dock_settings, "expanded");
	g_object_set((GObject *) self->widget,
		"expanded", expanded,
		NULL);
	g_object_set((GObject *) gel_app_get_window(app),
		"resizable", expanded,
		NULL);
	g_signal_connect(self->widget, "activate", (GCallback) expander_activate_cb, self);

	self->dock_idx = g_settings_get_strv(dock_settings, "widget-order");

	// Handle tab-reorder
	gtk_widget_realize(GTK_WIDGET(self->dock));
	g_signal_connect(self->dock, "page-reordered", G_CALLBACK(page_reorder_cb), self);

	// Remove preexistent tabs
	gint i;
	for (i = 0; i < gtk_notebook_get_n_pages(self->dock); i++)
		gtk_notebook_remove_page(self->dock, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	gtk_notebook_set_show_tabs(self->dock, FALSE);

	// Transfer widget to player
	GtkWidget *parent = gtk_widget_get_parent(self->widget);
	g_object_ref(self->widget);
	gtk_widget_show(self->widget);
	gtk_container_remove(GTK_CONTAINER(parent), self->widget);
	eina_window_add_widget(EINA_OBJ_GET_WINDOW(self), self->widget, TRUE, TRUE, 0);
	gtk_widget_destroy(parent);

	return TRUE;
}

G_MODULE_EXPORT gboolean
dock_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaDock *self = gel_app_shared_get(app, "dock");
	if (self == NULL)
		return FALSE;

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	gint pos = 0;
	while (self->dock_idx[pos] != NULL)
		if (g_str_equal(id, self->dock_idx[pos]))
			break;
		else
			pos++;

	if (!self->dock || (g_hash_table_lookup(self->dock_items, id) != NULL))
	{
		return FALSE;
	}

	if (gtk_notebook_append_page(self->dock, dock_widget, label) == -1)
	{
		g_hash_table_remove(self->dock_items, id);
		gel_error("Cannot add widget to dock");
		return FALSE;
	}
	g_hash_table_insert(self->dock_items, g_strdup(id), (gpointer) dock_widget);

	gel_info("Added dock '%s'", id);
	gtk_notebook_set_tab_reorderable(self->dock, dock_widget, TRUE);
	if (pos > -1)
	{
		gtk_notebook_reorder_child(self->dock, dock_widget, pos);
		if (pos <= gtk_notebook_get_current_page(self->dock))
			gtk_notebook_set_current_page(self->dock, pos);
	}

	if (gtk_notebook_get_n_pages(self->dock) > 1)
	{
		gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "dock-label"), N_("<big><b>Dock</b></big>"));
		gtk_notebook_set_show_tabs(self->dock, TRUE);
	}
	else
	{
		gchar *markup = g_strconcat("<big><b>", id, "</b></big>", NULL);
		gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "dock-label"), markup);
		g_free(markup);
	}

	return TRUE;
}

gboolean
eina_dock_remove_widget(EinaDock *self, gchar *id)
{
	GtkWidget *dock_item;

	if (!self->dock || ((dock_item = g_hash_table_lookup(self->dock_items, id)) == NULL))
	{
		return FALSE;
	}

	gtk_notebook_remove_page(GTK_NOTEBOOK(self->dock), gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item));

	GList *ids;
	gchar *markup;
	switch (gtk_notebook_get_n_pages(self->dock) <= 1)
	{
	case 0:
		gtk_widget_hide(self->widget);
		break;
	case 1:
		ids = g_hash_table_get_keys(self->dock_items);
		markup = g_strconcat("<big><b>", (gchar *) ids->data, "</b></big>", NULL);
		gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "dock-label"), markup);
		g_list_free(ids);
		g_free(markup);
		gtk_notebook_set_show_tabs(self->dock, FALSE);
		break;
	default:
		break;
	}

	return g_hash_table_remove(self->dock_items, id);
}

gboolean
eina_dock_switch_widget(EinaDock *self, gchar *id)
{
	gint page_num;
	GtkWidget *dock_item;

	dock_item = g_hash_table_lookup(self->dock_items, (gpointer) id);
	if (dock_item == NULL)
	{
		gel_error("Cannot find dock widget '%s'", id);
		return FALSE;
	}

	page_num = gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item);
	if (page_num == -1)
	{
		gel_error("Dock item %s is not in dock", id);
		return FALSE;
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->dock), page_num);
	return TRUE;
}

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self)
{
	gint n_tabs = gtk_notebook_get_n_pages(w);
	
	gchar **items = g_new0(gchar *, n_tabs + 1);
	gint j = 0;
	for (gint i = 0; i < n_tabs; i++)
	{
		gpointer tab = gtk_notebook_get_nth_page(w, i);

		GHashTableIter iter;
		gchar *id;
		gpointer _tab;
		g_hash_table_iter_init(&iter, self->dock_items);
		while (g_hash_table_iter_next(&iter, (gpointer *) &id, &_tab))
		{
			if (tab == _tab)
			{
				items[j++] = id;
				break;
			}
		}
	}

	GSettings *s = eina_obj_get_gsettings(self, EINA_DOMAIN".preferences.dock");
	g_settings_set_strv(s, "widget-order", (const gchar * const*) items);
	g_free(items);
}

static void
expander_activate_cb(GtkExpander *wi, EinaDock *self)
{
	gboolean resizable = !gtk_expander_get_expanded(wi);

	GtkWindow *window = (GtkWindow *) eina_obj_get_window(self);
	GSettings *dock_settings = gel_app_get_gsettings(eina_obj_get_app(self), EINA_DOMAIN".preferences.dock");

	g_settings_set_boolean(dock_settings, "expanded", resizable);
	gtk_window_set_resizable(window, resizable);

	if (!resizable)
	{
		gint w, h;
		gtk_window_get_size(window, &w, &h);
		g_settings_set_int(dock_settings, "window-width", w);
		g_settings_set_int(dock_settings, "window-height", h);
		g_signal_handlers_disconnect_by_func(self->dock, expander_expose_event, self);
	}
	else
	{
		g_signal_connect(self->dock, "expose-event", (GCallback) expander_expose_event, self);
	}
}

static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self)
{
	GSettings *dock_settings = gel_app_get_gsettings(eina_obj_get_app(self), EINA_DOMAIN".preferences.dock");
	gint w = g_settings_get_int(dock_settings, "window-width");
	gint h = g_settings_get_int(dock_settings, "window-height");
	gtk_window_resize((GtkWindow *) eina_obj_get_window(self), w, h);
	g_signal_handlers_disconnect_by_func(self->dock, expander_expose_event, self);

	return FALSE;
}

EINA_PLUGIN_INFO_SPEC(dock,
	NULL,						// version
	"settings,window",			// deps
	NULL,						// author
	NULL,						// url
	N_("Build-in dock plugin"), // short
	NULL,						// long
	NULL 						// icon
);

