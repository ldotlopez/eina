/*
 * eina/dock.c
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

#define GEL_DOMAIN "Eina::Dock"

#include <config.h>
#include <string.h>
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/dock.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>

struct _EinaDock {
	EinaObj      parent;

	GtkWidget   *widget;
	GtkNotebook *dock;
	GHashTable  *dock_items;
	GList       *dock_idx;

	gint         w, h;
};

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);

static gboolean
player_main_window_expose_event_cb(GtkWidget *w, GdkEventExpose *event, EinaDock *self);
static gboolean
player_main_window_configure_event_cb(GtkWidget *w, GdkEventConfigure *event, EinaDock *self);
static void
expander_activate_cb(GtkExpander *w, EinaDock *self);

static gboolean
dock_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaDock *self;

	self = g_new0(EinaDock, 1);
	if (!eina_obj_init(EINA_OBJ(self), app, "dock", EINA_OBJ_GTK_UI, error))
		return FALSE;

	if (!eina_obj_require(EINA_OBJ(self), "player", error))
	{
		eina_obj_unrequire(EINA_OBJ(self), "settings", error);
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	self->widget = eina_obj_get_widget(self, "dock-expander");
	self->dock   = eina_obj_get_typed(self, GTK_NOTEBOOK, "dock-notebook");

	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);

	// Delete old keys
	eina_conf_delete_key(conf, "/ui/size_w");
	eina_conf_delete_key(conf, "/ui/size_h");
	eina_conf_delete_key(conf, "/ui/main-window/width");
	eina_conf_delete_key(conf, "/ui/main-window/height");

	// Configure the dock route table
	const gchar *order;
	gchar **split;

	order = eina_conf_get_str(conf, "/dock/order", "playlist");
	gel_list_deep_free(self->dock_idx, g_free);
	split = g_strsplit(order, ",", 0);
	self->dock_idx = gel_strv_to_list(split, FALSE);
	g_free(split); // NOT g_freestrv, look the FALSE in the prev. line

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
	GtkWidget *parent = self->widget->parent;
	g_object_ref(self->widget);
	gtk_widget_show(self->widget);
	gtk_container_remove(GTK_CONTAINER(parent), self->widget);
	eina_player_add_widget(EINA_OBJ_GET_PLAYER(self), self->widget);
	gtk_widget_destroy(parent);

	// On first expose restore settings, connect expander signal, disconnect
	// myself
	g_signal_connect(
		eina_player_get_main_window(EINA_OBJ_GET_PLAYER(self)), "expose-event",
		(GCallback) player_main_window_expose_event_cb, self);

	return TRUE;
}

static gboolean
dock_exit(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaDock *self = gel_app_shared_get(app, "dock");
	if (self == NULL)
		return FALSE;

	eina_obj_unrequire(EINA_OBJ(self), "player", NULL);

	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	gint pos = g_list_position(self->dock_idx,
		g_list_find_custom(self->dock_idx, id, (GCompareFunc) strcmp));

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
		gtk_notebook_set_show_tabs(self->dock, TRUE);

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

	switch (gtk_notebook_get_n_pages(self->dock) <= 1)
	{
	case 0:
		gtk_widget_hide(self->widget);
		break;
	case 1:
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
page_reorder_cb_aux (gpointer k, gpointer v, gpointer data)
{
	GList *list       = (GList *) data;
	GList *p;

	p = g_list_find(list, v);
	if (p == NULL)
		return;
	
	p->data = k;
}

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self)
{
	gint n_tabs, i;
	GList *dock_items = NULL;
	GString *output = g_string_new(NULL);

	n_tabs = gtk_notebook_get_n_pages(w);
	for (i = (n_tabs - 1); i >= 0; i--)
	{
		dock_items = g_list_prepend(dock_items, gtk_notebook_get_nth_page(w, i));
	}

	if (self->dock_items != NULL)
		g_hash_table_foreach(self->dock_items, page_reorder_cb_aux, dock_items);
	for (i = 0; i < n_tabs; i++)
	{
		output = g_string_append(output, (gchar *) g_list_nth_data(dock_items, i));
		if ((i+1) < n_tabs)
			output = g_string_append_c(output, ',');
	}

	eina_conf_set_str(EINA_OBJ_GET_SETTINGS(self), "/dock/order", output->str);
	g_string_free(output, TRUE);
}

static gboolean
player_main_window_expose_event_cb(GtkWidget *w, GdkEventExpose *event, EinaDock *self)
{
	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);
	gboolean expanded = eina_conf_get_bool(conf, "/dock/expanded", FALSE);
	self->w = eina_conf_get_int(conf, "/dock/main-window-width",  1);
	self->h = eina_conf_get_int(conf, "/dock/main-window-height", 1);

	gtk_expander_set_expanded((GtkExpander*) self->widget, expanded);
	gtk_window_set_resizable((GtkWindow *) w, expanded);
	gtk_window_resize((GtkWindow *) w, self->w, self->h);

	if (expanded)
		g_signal_connect(w, "configure-event",
			(GCallback) player_main_window_configure_event_cb, self);

	g_signal_connect(self->widget, "activate", (GCallback) expander_activate_cb, self);

	g_signal_handlers_disconnect_by_func(w, player_main_window_expose_event_cb, self);
	return FALSE;
}

static gboolean
player_main_window_configure_event_cb(GtkWidget *w, GdkEventConfigure *event, EinaDock *self)
{
	if (event->type != GDK_CONFIGURE)
		return FALSE;
	self->w = event->width;
	self->h = event->height;

	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);
	eina_conf_set_int(conf, "/dock/main-window-width", self->w);
	eina_conf_set_int(conf, "/dock/main-window-height", self->h);

	return FALSE;
}

static void
expander_activate_cb(GtkExpander *w, EinaDock *self)
{
	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);
	gboolean expanded = !gtk_expander_get_expanded((GtkExpander *) self->widget);
	GtkWindow *win = (GtkWindow *) eina_player_get_main_window(EINA_OBJ_GET_PLAYER(self));

	eina_conf_set_bool(conf, "/dock/expanded", expanded);
	if (expanded)
	{
		gtk_window_set_resizable(win, TRUE);
		gtk_window_resize(win, self->w, self->h);
		g_signal_connect(eina_player_get_main_window(EINA_OBJ_GET_PLAYER(self)), "configure-event",
			(GCallback) player_main_window_configure_event_cb, self);
	}
	else
	{
		g_signal_handlers_disconnect_by_func(eina_player_get_main_window(EINA_OBJ_GET_PLAYER(self)),
			player_main_window_configure_event_cb,
			self);
		gtk_window_set_resizable(win, FALSE);
	}
}

G_MODULE_EXPORT GelPlugin dock_plugin = {
	GEL_PLUGIN_SERIAL,
	"dock", PACKAGE_VERSION,
	N_("Build-in dock plugin"), NULL,
	NULL, NULL, NULL, 
	dock_init, dock_exit,
	NULL, NULL
};

