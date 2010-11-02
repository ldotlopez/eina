/*
 * eina/dock/eina-dock.c
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
#include <gel/gel.h>

G_DEFINE_TYPE (EinaDock, eina_dock, GTK_TYPE_EXPANDER)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_DOCK, EinaDockPrivate))

#define DOCK_DEFAULT_LABEL N_("<big><b>Dock</b></big>")

typedef struct _EinaDockPrivate EinaDockPrivate;

static void
dock_update_properties(EinaDock *self);
static void
dock_reorder_pages(EinaDock *self);

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);

GEL_DEFINE_WEAK_REF_CALLBACK(eina_dock)

enum {
	PROP_PAGE_ORDER = 1
};

struct _EinaDockPrivate {
	GtkNotebook *notebook;
	GHashTable  *id2widget; // gchar* -> GtkWidget
	GHashTable  *widget2id; // GtkWidget -> gchar*

	gchar      **page_order;   // Prop->page-order

	gint         w, h;
};

static void
eina_dock_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PAGE_ORDER:
		g_value_set_boxed(value, eina_dock_get_page_order((EinaDock *) object));
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
	EinaDock *self = EINA_DOCK(object);
	EinaDockPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->id2widget, NULL, g_hash_table_destroy);
	gel_free_and_invalidate(priv->widget2id, NULL, g_hash_table_destroy);
	gel_free_and_invalidate(priv->page_order, NULL, g_strfreev);

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

	priv->id2widget = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	priv->widget2id = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	priv->page_order = NULL;
	
	gtk_expander_set_use_markup(GTK_EXPANDER(self), TRUE);
	gtk_container_add(GTK_CONTAINER(self), (GtkWidget *) priv->notebook);

	dock_update_properties(self);
	g_signal_connect(priv->notebook, "page-reordered", G_CALLBACK(page_reorder_cb), self);
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
	return gel_strv_copy(GET_PRIVATE(self)->page_order, TRUE);
}

void
eina_dock_set_page_order(EinaDock *self, gchar **order)
{	
	g_return_if_fail(EINA_IS_DOCK(self));
	g_return_if_fail(order);

	EinaDockPrivate *priv = GET_PRIVATE(self);
	g_strfreev(priv->page_order);
	priv->page_order = gel_strv_copy(order, TRUE);

	dock_reorder_pages(self);

	g_object_notify((GObject *) self, "page-order");
}

static void
dock_update_properties(EinaDock *self)
{
	g_return_if_fail(EINA_IS_DOCK(self));
	
	EinaDockPrivate *priv = GET_PRIVATE(self);
	guint n_pages = gtk_notebook_get_n_pages(priv->notebook);

	gboolean visible = FALSE;
	gboolean show_tabs = FALSE;
	switch (n_pages)
	{
	case 0:
		visible = FALSE;
		break;
	case 1:
		visible = TRUE;
		show_tabs = FALSE;
		break;
	default:
		visible   = TRUE;
		show_tabs = TRUE;
		break;
	}
	if (visible)
		gtk_widget_show(GTK_WIDGET(self));
	else
		gtk_widget_hide(GTK_WIDGET(self));

	g_object_set((GObject *) priv->notebook,
		"show-tabs", show_tabs,
		"show-border", show_tabs,
		NULL);

	if (visible)
	{
		if (show_tabs)
			gtk_expander_set_label(GTK_EXPANDER(self), DOCK_DEFAULT_LABEL);
		else
		{
			GList *k = g_hash_table_get_keys(priv->id2widget);
			gchar *markup = g_strconcat("<big><b>", (gchar *) k->data, "</b></big>", NULL);
			g_list_free(k);
			gtk_expander_set_label(GTK_EXPANDER(self), markup);
			g_free(markup);
		}
	}
}

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(label) && GTK_IS_WIDGET(dock_widget), FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(g_hash_table_lookup(priv->id2widget, id) == NULL, FALSE);

	gint pos = 0;
	while (priv->page_order && (priv->page_order[pos] != NULL))
		if (g_str_equal(id, priv->page_order[pos]))
			break;
		else
			pos++;

	if (gtk_notebook_append_page(priv->notebook, dock_widget, label) == -1)
	{
		g_warning(N_("Cannot add widget to dock"));
		return FALSE;
	}
	g_object_weak_ref((GObject *) dock_widget, eina_dock_weak_ref_cb, NULL);

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

	g_hash_table_insert(priv->id2widget, g_strdup(id), dock_widget);
	g_hash_table_insert(priv->widget2id, dock_widget, g_strdup(id));

	gtk_widget_show(label);
	gtk_widget_show(dock_widget);

	gtk_notebook_set_tab_reorderable(priv->notebook, dock_widget, TRUE);

	dock_update_properties(self);

	return TRUE;
}

gboolean
eina_dock_remove_widget(EinaDock *self, GtkWidget *w)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(w), FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);

	gchar *id = (gchar *) g_hash_table_lookup(priv->widget2id, w);
	if (id == NULL)
	{
		g_warning(N_("Unable to find id for widget %p of type %s"), w, G_OBJECT_TYPE_NAME(w));
		g_return_val_if_fail(id != NULL, FALSE);
	}

	g_object_weak_unref((GObject *) w, eina_dock_weak_ref_cb, NULL);
	gtk_notebook_remove_page(priv->notebook, gtk_notebook_page_num(priv->notebook, w));
	g_hash_table_remove(priv->id2widget, id);
	g_hash_table_remove(priv->widget2id, w);

	dock_update_properties(self);

	return TRUE;
}

gboolean
eina_dock_remove_widget_by_id(EinaDock *self, gchar *id)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id, FALSE);
	return eina_dock_remove_widget(self, g_hash_table_lookup(GET_PRIVATE(self)->id2widget, id));
}

gboolean
eina_dock_switch_widget(EinaDock *self, gchar *id)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	EinaDockPrivate *priv = GET_PRIVATE(self);

	GtkWidget *dock_item = g_hash_table_lookup(priv->id2widget, (gpointer) id);
	g_return_val_if_fail(dock_item != NULL, FALSE);

	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(priv->notebook), dock_item);
	g_return_val_if_fail(page_num >= 0, FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
	return TRUE;
}

static void
dock_reorder_pages(EinaDock *self)
{
	g_return_if_fail(EINA_IS_DOCK(self));
	EinaDockPrivate *priv = GET_PRIVATE(self);

	gint pos = 0;
	for (guint i = 0; priv->page_order && priv->page_order[i]; i++)
	{
		GtkWidget *w = GTK_WIDGET(g_hash_table_lookup(priv->id2widget, priv->page_order[i]));
		if (w)
			gtk_notebook_reorder_child(priv->notebook, w, pos++);
	}
}

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self)
{
	EinaDockPrivate *priv = GET_PRIVATE(self);

	gint n_tabs = gtk_notebook_get_n_pages(w);
	
	gchar **items = g_new0(gchar *, n_tabs + 1);
	gint j = 0;
	for (gint i = 0; i < n_tabs; i++)
	{
		gpointer tab = gtk_notebook_get_nth_page(w, i);

		GHashTableIter iter;
		gchar *id;
		gpointer _tab;
		g_hash_table_iter_init(&iter, priv->id2widget);
		while (g_hash_table_iter_next(&iter, (gpointer *) &id, &_tab))
		{
			if (tab == _tab)
			{
				items[j++] = g_strdup(id);
				break;
			}
		}
	}

	g_strfreev(priv->page_order);
	priv->page_order = items;

	g_object_notify((GObject *) self, "page-order");
}

#if 0
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
#endif

