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

	GtkWidget   *expander;
	GtkNotebook *notebook;
	GHashTable  *dock_items;
	gchar     **dock_idx;
	
	gint         w, h;
};

static void 
dock_reorder(EinaDock *self);

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);
static void
expander_activate_cb(GtkExpander *wi, EinaDock *self);
static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self);

static void
settings_changed_cb(GSettings *setting, gchar *key, EinaDock *self);

G_MODULE_EXPORT gboolean
dock_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaDock *self;

	self = g_new0(EinaDock, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "dock", EINA_OBJ_GTK_UI, error))
		return FALSE;

	self->expander = eina_obj_get_widget(self, "dock-expander");
	self->notebook   = eina_obj_get_typed(self, GTK_NOTEBOOK, "dock-notebook");

	/*
	 * Setup dock
	 */
	GSettings *settings = gel_app_get_settings(app, EINA_DOCK_PREFERENCES_DOMAIN);
	gboolean expanded = g_settings_get_boolean(settings, EINA_DOCK_EXPANDED_KEY);
	g_object_set((GObject *) self->expander,
		"expanded", expanded,
		NULL);
	g_object_set((GObject *) gel_app_get_window(app),
		"resizable", expanded,
		NULL);
	g_signal_connect(self->expander, "activate", (GCallback) expander_activate_cb, self);

	/*
	 * Setup tab ordering
	 */
	gtk_widget_realize(GTK_WIDGET(self->notebook));

	// Remove preexistent tabs
	gint i;
	for (i = 0; i < gtk_notebook_get_n_pages(self->notebook); i++)
		gtk_notebook_remove_page(self->notebook, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	gtk_notebook_set_show_tabs(self->notebook, FALSE);

	// Transfer widget to player
	GtkWidget *parent = gtk_widget_get_parent(self->expander);
	g_object_ref(self->expander);
	gtk_widget_show(self->expander);
	gtk_container_remove(GTK_CONTAINER(parent), self->expander);
	eina_window_add_widget(EINA_OBJ_GET_WINDOW(self), self->expander, TRUE, TRUE, 0);
	gtk_widget_destroy(parent);

	self->dock_idx = g_settings_get_strv(settings, EINA_DOCK_WIDGET_ORDER_KEY);
	g_signal_connect(self->notebook, "page-reordered", G_CALLBACK(page_reorder_cb), self);

	/*
	 * Listen settings changes
	 */
	g_signal_connect(settings, "changed", (GCallback) settings_changed_cb, self);

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

	if (!self->notebook || (g_hash_table_lookup(self->dock_items, id) != NULL))
	{
		return FALSE;
	}

	if (gtk_notebook_append_page(self->notebook, dock_widget, label) == -1)
	{
		g_hash_table_remove(self->dock_items, id);
		gel_error("Cannot add widget to dock");
		return FALSE;
	}
	g_hash_table_insert(self->dock_items, g_strdup(id), (gpointer) dock_widget);

	gel_info("Added dock '%s'", id);
	gtk_notebook_set_tab_reorderable(self->notebook, dock_widget, TRUE);
	if (pos > -1)
	{
		gtk_notebook_reorder_child(self->notebook, dock_widget, pos);
		if (pos <= gtk_notebook_get_current_page(self->notebook))
			gtk_notebook_set_current_page(self->notebook, pos);
	}

	if (gtk_notebook_get_n_pages(self->notebook) > 1)
	{
		gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "dock-label"), N_("<big><b>Dock</b></big>"));
		gtk_notebook_set_show_tabs(self->notebook, TRUE);
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

	if (!self->notebook || ((dock_item = g_hash_table_lookup(self->dock_items, id)) == NULL))
	{
		return FALSE;
	}

	gtk_notebook_remove_page(GTK_NOTEBOOK(self->notebook), gtk_notebook_page_num(GTK_NOTEBOOK(self->notebook), dock_item));

	GList *ids;
	gchar *markup;
	switch (gtk_notebook_get_n_pages(self->notebook) <= 1)
	{
	case 0:
		gtk_widget_hide(self->expander);
		break;
	case 1:
		ids = g_hash_table_get_keys(self->dock_items);
		markup = g_strconcat("<big><b>", (gchar *) ids->data, "</b></big>", NULL);
		gtk_label_set_markup(eina_obj_get_typed(self, GTK_LABEL, "dock-label"), markup);
		g_list_free(ids);
		g_free(markup);
		gtk_notebook_set_show_tabs(self->notebook, FALSE);
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

	page_num = gtk_notebook_page_num(GTK_NOTEBOOK(self->notebook), dock_item);
	if (page_num == -1)
	{
		gel_error("Dock item %s is not in dock", id);
		return FALSE;
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook), page_num);
	return TRUE;
}

static void 
dock_reorder(EinaDock *self)
{
	for (guint i = 0; self->dock_idx[i] != NULL; i++)
	{
		GtkWidget *tab = gtk_notebook_get_nth_page(self->notebook, i);
		gtk_notebook_reorder_child(self->notebook, tab, i);
	}
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

	GSettings *s = eina_obj_get_settings(self, EINA_DOCK_PREFERENCES_DOMAIN);
	g_settings_set_strv(s, EINA_DOCK_WIDGET_ORDER_KEY, (const gchar * const*) items);
	g_free(items);
}

static void
expander_activate_cb(GtkExpander *wi, EinaDock *self)
{
	gboolean resizable = !gtk_expander_get_expanded(wi);

	GtkWindow *window = (GtkWindow *) eina_obj_get_window(self);
	GSettings *dock_settings = gel_app_get_settings(eina_obj_get_app(self), EINA_DOCK_PREFERENCES_DOMAIN);

	g_settings_set_boolean(dock_settings, EINA_DOCK_EXPANDED_KEY, resizable);
	gtk_window_set_resizable(window, resizable);

	if (!resizable)
	{
		gint w, h;
		gtk_window_get_size(window, &w, &h);
		g_settings_set_int(dock_settings, EINA_DOCK_WINDOW_W_KEY, w);
		g_settings_set_int(dock_settings, EINA_DOCK_WINDOW_H_KEY, h);
		g_signal_handlers_disconnect_by_func(self->notebook, expander_expose_event, self);
	}
	else
	{
		g_signal_connect(self->notebook, "expose-event", (GCallback) expander_expose_event, self);
	}
}

static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self)
{
	GSettings *dock_settings = gel_app_get_settings(eina_obj_get_app(self), EINA_DOCK_PREFERENCES_DOMAIN);
	gint w = g_settings_get_int(dock_settings, "window-width");
	gint h = g_settings_get_int(dock_settings, "window-height");
	gtk_window_resize((GtkWindow *) eina_obj_get_window(self), w, h);
	g_signal_handlers_disconnect_by_func(self->notebook, expander_expose_event, self);

	return FALSE;
}

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaDock *self)
{
	if (g_str_equal(key, EINA_DOCK_EXPANDED_KEY))
	{
		gtk_expander_set_expanded((GtkExpander *) self->expander, g_settings_get_boolean(settings, key));
	}

	else if (g_str_equal(key, EINA_DOCK_WIDGET_ORDER_KEY))
	{
		g_strfreev(self->dock_idx);
		self->dock_idx = gel_strv_copy(g_settings_get_strv(settings, key), TRUE);
		dock_reorder(self);
	}

	else
	{
		g_warning(N_("Unhanded key '%s'"), key);
	}
}

EINA_PLUGIN_INFO_SPEC(dock,
	NULL,						// version
	"window",		            // deps
	NULL,						// author
	NULL,						// url
	N_("Build-in dock plugin"), // short
	NULL,						// long
	NULL 						// icon
);

