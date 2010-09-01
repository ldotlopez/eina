/*
 * plugins/dock/eina-dock.c
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

#include "eina-dock.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaDock, eina_dock, GTK_TYPE_EXPANDER)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_DOCK, EinaDockPrivate))

#define DOCK_DEFAULT_LABEL N_("<big><b>Dock</b></big>")

typedef struct _EinaDockPrivate EinaDockPrivate;

enum {
	PROP_PAGE_ORDER = 1
};

struct _EinaDockPrivate {
	GtkNotebook *notebook;
	GHashTable  *dock_items;
	gchar      **dock_idx;

	gint         w, h;
};

static void
eina_dock_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PAGE_ORDER:
		g_value_set_static_boxed(value, eina_dock_get_page_order((EinaDock *) object));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_dock_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PAGE_ORDER:
		eina_dock_set_page_order((EinaDock *) object, g_value_get_boxed(value));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_dock_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_dock_parent_class)->dispose (object);
}

static void
eina_dock_class_init (EinaDockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaDockPrivate));

	object_class->get_property = eina_dock_get_property;
	object_class->set_property = eina_dock_set_property;
	object_class->dispose = eina_dock_dispose;

	g_object_class_install_property(object_class, PROP_PAGE_ORDER,
		g_param_spec_boxed("page-order", "page-order", "page-order",
		G_TYPE_STRV, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
eina_dock_init (EinaDock *self)
{
	EinaDockPrivate *priv = GET_PRIVATE(self);

	priv->notebook = (GtkNotebook *) gtk_notebook_new();
	g_object_set((GObject *) priv->notebook,
		"show-border", FALSE,
		"show-tabs", FALSE,
		"tab-pos", GTK_POS_LEFT,
		NULL);
	gtk_widget_show((GtkWidget *) priv->notebook);

	priv->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	gtk_expander_set_use_markup(GTK_EXPANDER(self), TRUE);
	gtk_container_add(GTK_CONTAINER(self), (GtkWidget *) priv->notebook);
}

EinaDock*
eina_dock_new (void)
{
	return g_object_new (EINA_TYPE_DOCK, 
		"use-markup", TRUE,
		"label", DOCK_DEFAULT_LABEL,
		NULL);
}

gchar **
eina_dock_get_page_order(EinaDock *self)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), NULL);
	g_warning(N_("Function %s not implemented"), __FUNCTION__);
	return NULL;
}

void
eina_dock_set_page_order(EinaDock *self, gchar **order)
{	
	g_return_if_fail(EINA_IS_DOCK(self));
	g_return_if_fail(order);

	g_warning(N_("Function %s not implemented"), __FUNCTION__);
	g_object_notify((GObject *) self, "page-order");
}

#if 0
static void 
dock_reorder(EinaDock *self);

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);
#if !OSX_SYSTEM
static void
expander_activate_cb(GtkExpander *wi, EinaDock *self);
static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self);
#endif

static void
settings_changed_cb(GSettings *setting, gchar *key, EinaDock *self);
#endif

#if 0
	self->expander = eina_obj_get_widget(self, "dock-expander");
	priv->notebook = eina_obj_get_typed(self, GTK_NOTEBOOK, "dock-notebook");

	/*
	 * Setup dock
	 */
	GSettings *settings = gel_app_get_settings(app, EINA_DOCK_PREFERENCES_DOMAIN);
	#if !OSX_SYSTEM
	gboolean expanded = g_settings_get_boolean(settings, EINA_DOCK_EXPANDED_KEY);
	g_object_set((GObject *) self->expander,
		"expanded", expanded,
		NULL);
	g_object_set((GObject *) gel_app_get_window(app),
		"resizable", expanded,
		NULL);
	g_signal_connect(self->expander, "activate", (GCallback) expander_activate_cb, self);
	#else
	g_object_set((GObject *) gel_app_get_window(app), "resizable", TRUE, NULL);
	#endif

	/*
	 * Setup tab ordering
	 */
	gtk_widget_realize(GTK_WIDGET(priv->notebook));

	// Remove preexistent tabs
	gint i;
	for (i = 0; i < gtk_notebook_get_n_pages(priv->notebook); i++)
		gtk_notebook_remove_page(priv->notebook, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	gtk_notebook_set_show_tabs(priv->notebook, FALSE);

	// Transfer widget to player
	#if !OSX_SYSTEM
	GtkWidget *dock_widget = self->expander;
	#else
	GtkWidget *dock_widget = (GtkWidget *) priv->notebook;
	#endif

	GtkWidget *parent = gtk_widget_get_parent(dock_widget);
	g_object_ref(dock_widget);
	gtk_widget_show(dock_widget);
	gtk_container_remove(GTK_CONTAINER(parent), dock_widget);
	eina_window_add_widget(EINA_OBJ_GET_WINDOW(self), dock_widget, TRUE, TRUE, 0);
	gtk_widget_destroy(parent);

	self->dock_idx = g_settings_get_strv(settings, EINA_DOCK_WIDGET_ORDER_KEY);
	g_signal_connect(priv->notebook, "page-reordered", G_CALLBACK(page_reorder_cb), self);

	/*
	 * Listen settings changes
	 */
	g_signal_connect(settings, "changed", (GCallback) settings_changed_cb, self);

	return TRUE;
}
#endif

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(label) && GTK_IS_WIDGET(dock_widget), FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);

	g_return_val_if_fail(g_hash_table_lookup(priv->dock_items, id) == NULL, FALSE);

	gint pos = 0;
	while (priv->dock_idx && (priv->dock_idx[pos] != NULL))
		if (g_str_equal(id, priv->dock_idx[pos]))
			break;
		else
			pos++;

	if (GTK_IS_LABEL(label))
	{
		gdouble angle = 0;
		switch (gtk_notebook_get_tab_pos(priv->notebook))
		{
		case GTK_POS_TOP:
			angle = 0;
			break;
		case GTK_POS_RIGHT:
			angle = 270;
			break;
		case GTK_POS_BOTTOM:
			angle = 0;
			break;
		case GTK_POS_LEFT:
			angle = 90;
			break;
		}
		gtk_label_set_angle(GTK_LABEL(label), angle);
	}
	if (gtk_notebook_append_page(priv->notebook, dock_widget, label) == -1)
	{
		g_hash_table_remove(priv->dock_items, id);
		g_warning(N_("Cannot add widget to dock"));
		return FALSE;
	}
	gtk_notebook_reorder_child(priv->notebook, dock_widget, -1);

	gtk_widget_show(label);
	gtk_widget_show(dock_widget);
	gtk_notebook_set_tab_reorderable(priv->notebook, dock_widget, TRUE);
	g_hash_table_insert(priv->dock_items, g_strdup(id), (gpointer) dock_widget);

	if (pos > -1)
	{
		gtk_notebook_reorder_child(priv->notebook, dock_widget, pos);
		if (pos <= gtk_notebook_get_current_page(priv->notebook))
			gtk_notebook_set_current_page(priv->notebook, pos);
	}

	// Set some properties based on multiple pages
	gboolean multiple_pages = gtk_notebook_get_n_pages(priv->notebook) > 1;
	g_object_set((GObject *) priv->notebook,
		"show-tabs", multiple_pages,
		"show-border", multiple_pages,
		NULL);

	if (multiple_pages)
		gtk_expander_set_label(GTK_EXPANDER(self), DOCK_DEFAULT_LABEL);
	else
	{
		gchar *markup = g_strconcat("<big><b>", id, "</b></big>", NULL);
		gtk_expander_set_label(GTK_EXPANDER(self), markup);
		g_free(markup);
	}

	return TRUE;
}

gboolean
eina_dock_remove_widget(EinaDock *self, gchar *id)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);

	GtkWidget *dock_item = g_hash_table_lookup(priv->dock_items, id);
	g_return_val_if_fail(dock_item != NULL, FALSE);

	gtk_notebook_remove_page(GTK_NOTEBOOK(priv->notebook), gtk_notebook_page_num(GTK_NOTEBOOK(priv->notebook), dock_item));

	GList *ids;
	gchar *markup;
	switch (gtk_notebook_get_n_pages(priv->notebook) <= 1)
	{
	case 0:
		gtk_widget_hide((GtkWidget *) self);
		break;

	case 1:
		ids = g_hash_table_get_keys(priv->dock_items);
		markup = g_strconcat("<big><b>", (gchar *) ids->data, "</b></big>", NULL);
		gtk_expander_set_label(GTK_EXPANDER(self), markup);
		g_list_free(ids);
		g_free(markup);
		gtk_notebook_set_show_tabs(priv->notebook, FALSE);
		break;
	default:
		break;
	}

	return g_hash_table_remove(priv->dock_items, id);
}

gboolean
eina_dock_switch_widget(EinaDock *self, gchar *id)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);

	GtkWidget *dock_item = g_hash_table_lookup(priv->dock_items, (gpointer) id);
	g_return_val_if_fail(dock_item != NULL, FALSE);

	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(priv->notebook), dock_item);
	g_return_val_if_fail(page_num >= 0, FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
	return TRUE;
}

#if 0
static void 
dock_reorder(EinaDock *self)
{
	for (guint i = 0; self->dock_idx[i] != NULL; i++)
	{
		GtkWidget *tab = gtk_notebook_get_nth_page(priv->notebook, i);
		gtk_notebook_reorder_child(priv->notebook, tab, i);
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

#if !OSX_SYSTEM
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
		g_signal_handlers_disconnect_by_func(priv->notebook, expander_expose_event, self);
	}
	else
	{
		g_signal_connect(priv->notebook, "expose-event", (GCallback) expander_expose_event, self);
	}
}

static gboolean
expander_expose_event(GtkExpander *wi, GdkEventExpose *ev, EinaDock *self)
{
	GSettings *dock_settings = gel_app_get_settings(eina_obj_get_app(self), EINA_DOCK_PREFERENCES_DOMAIN);
	gint w = g_settings_get_int(dock_settings, "window-width");
	gint h = g_settings_get_int(dock_settings, "window-height");
	gtk_window_resize((GtkWindow *) eina_obj_get_window(self), w, h);
	g_signal_handlers_disconnect_by_func(priv->notebook, expander_expose_event, self);

	return FALSE;
}
#endif

static void
settings_changed_cb(GSettings *settings, gchar *key, EinaDock *self)
{
	#if !OSX_SYSTEM
	if (g_str_equal(key, EINA_DOCK_EXPANDED_KEY))
	{
		gtk_expander_set_expanded((GtkExpander *) self->expander, g_settings_get_boolean(settings, key));
		return;
	}
	#endif

	if (g_str_equal(key, EINA_DOCK_WIDGET_ORDER_KEY))
	{
		g_strfreev(self->dock_idx);
		self->dock_idx = gel_strv_copy(g_settings_get_strv(settings, key), TRUE);
		dock_reorder(self);
		return;
	}

	if (g_str_equal(key, EINA_DOCK_WINDOW_W_KEY) ||
	    g_str_equal(key, EINA_DOCK_WINDOW_H_KEY))
	{
		g_warning(N_("Unimplemented feature: resize window"));
		return;
	}

	else
	{
		g_warning(N_("Unhanded key '%s'"), key);
	}
}
#endif
