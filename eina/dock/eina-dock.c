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
#include <eina/dock/eina-dock-tab.h>

G_DEFINE_TYPE (EinaDock, eina_dock, GTK_TYPE_BOX)

#define EXPANDER_DEFAULT_LABEL N_("<big><b>Dock</b></big>")

#define dock_is_visible(dock)   (dock->priv->n_tabs >= 1)
#define dock_has_expander(dock) (dock->priv->n_tabs >= 2)
#define dock_has_tabs(dock)     (dock->priv->n_tabs >  2)

enum
{
	PROP_PAGE_ORDER = 1,
	PROP_RESIZABLE
};

struct _EinaDockPrivate {
	GtkExpander *expander;
	GtkNotebook *notebook;

	EinaDockTab *primary_tab;
	GList       *tabs;
	guint        n_tabs;

	gchar      **page_order;   // Prop->page-order
	gboolean     resizable;    // Prop->resizable

	gint         w, h;
};


static void
dock_update_properties(EinaDock *self);
static void
dock_reorder_pages(EinaDock *self);

static void
expander_notify_cb(GtkExpander *w, GParamSpec *pspec, EinaDock *self);
static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self);

enum
{
	WIDGET_ADD,
	WIDGET_REMOVE,
	WIDGET_N_SIGNALS
};
guint dock_signals[WIDGET_N_SIGNALS] = { 0 };

GEL_DEFINE_WEAK_REF_CALLBACK(eina_dock)

static void
eina_dock_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PAGE_ORDER:
		g_value_set_boxed(value, eina_dock_get_page_order((EinaDock *) object));
		return;

	case PROP_RESIZABLE:
		g_value_set_boolean(value, eina_dock_get_resizable((EinaDock *) object));
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
	EinaDockPrivate *priv = self->priv;

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

	/*
	 * EinaDock::page-order:
	 *
	 * Order for the tabs in the dock, based on they ids
	 */
	g_object_class_install_property(object_class, PROP_PAGE_ORDER,
		g_param_spec_boxed("page-order", "page-order", "page-order",
		G_TYPE_STRV, G_PARAM_READABLE | G_PARAM_WRITABLE));

	/*
	 * EinaDock::resizable:
	 *
	 * Whatever the dock is resizable or not. Matches the expander property
	 * from the internat #GtkNotebook
	 */
	g_object_class_install_property(object_class, PROP_RESIZABLE,
		g_param_spec_boolean("resizable", "resizable", "resizable",
		FALSE, G_PARAM_READABLE));

	/*
	 * EinaDock::widget-add:
	 * @dock: the #EinaDock object
	 * @id: the ID from the widget
	 *
	 * Emitted when a new widget is added to dock
	 */
	dock_signals[WIDGET_ADD] = g_signal_new("widget-add",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaDockClass, widget_add),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE,
		1,
		G_TYPE_STRING);

	/*
	 * EinaDock::widget-remove:
	 * @dock: the #EinaDock object
	 * @id: the ID from the widget
	 *
	 * Emitted when a widget is removed from the dock. This signal is emited
	 * after the widget was removed
	 */
	dock_signals[WIDGET_REMOVE] = g_signal_new("widget-remove",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaDockClass, widget_remove),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE,
		1,
		G_TYPE_STRING);
}

static void
eina_dock_init (EinaDock *self)
{
	EinaDockPrivate *priv =	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_DOCK, EinaDockPrivate));

	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
	g_object_set((GObject *) self,
		"spacing", 0,
		"border-width", 5,
		"homogeneous", FALSE,
		NULL);

	// Setup subwidgets
	g_object_set((GObject *) (priv->expander = (GtkExpander *) gtk_expander_new(NULL)),
		"use-markup", TRUE,
		"visible", FALSE,
		NULL);
	g_object_set((GObject *) (priv->notebook = (GtkNotebook *) gtk_notebook_new()),
		"show-border", FALSE,
		"show-tabs", FALSE,
		"tab-pos", GTK_POS_LEFT,
		"visible", TRUE,
		NULL);

	// Pack them
	gtk_container_add((GtkContainer *) priv->expander,  (GtkWidget *) priv->notebook);
	gtk_box_pack_start((GtkBox *) self,  (GtkWidget *) priv->expander, TRUE, TRUE, 0);

	priv->tabs = NULL;
	priv->n_tabs = 0;
	priv->page_order = NULL;

	g_signal_connect_after(priv->expander, "notify::expanded", G_CALLBACK(expander_notify_cb), self);
	g_signal_connect(priv->notebook, "page-reordered", G_CALLBACK(page_reorder_cb), self);
}

EinaDock*
eina_dock_new (void)
{
	return g_object_new (EINA_TYPE_DOCK, NULL);
}

gint
list_find_by_widget(EinaDockTab *tab, GtkWidget *w)
{
	return (eina_dock_tab_get_widget(tab) == w) ? 0 : 1;
}

gint
list_find_by_id(EinaDockTab *tab, const gchar *id)
{
	return g_str_equal(eina_dock_tab_get_id(tab), id) ? 0 : 1;
}

gint
list_find_by_tab(EinaDockTab *tab, EinaDockTab *tab_)
{
	return ((tab == tab_) || eina_dock_tab_equal(tab, tab_)) ? 0 : 1;
}

static void
dock_update_properties(EinaDock *self)
{
	g_return_if_fail(EINA_IS_DOCK(self));
	EinaDockPrivate *priv = self->priv;

	// Update misc properties
	
	g_object_set((GObject *) self,
		"visible", dock_is_visible(self),
		NULL);

	g_object_set((GObject *) priv->expander,
		"visible", dock_has_expander(self),
		NULL);

	g_object_set((GObject *) priv->notebook,
		"show-tabs",   dock_has_tabs(self),
		"show-border", dock_has_tabs(self),
		NULL);

	// Update expander markup

	if (dock_has_expander(self) && dock_has_tabs(self))
		gtk_expander_set_label(priv->expander, EXPANDER_DEFAULT_LABEL);

	if (dock_has_expander(self) && !dock_has_tabs(self))
	{
		GtkWidget *current_widget = gtk_notebook_get_nth_page(priv->notebook, gtk_notebook_get_current_page(priv->notebook));
		GList *tab_l = g_list_find_custom(priv->tabs, current_widget, (GCompareFunc) list_find_by_widget);
		g_warn_if_fail(tab_l != NULL);
		if (tab_l)
		{
			gchar *markup = g_strconcat("<big><b>", eina_dock_tab_get_id((EinaDockTab *) tab_l->data), "</b></big>", NULL);
			gtk_expander_set_label(priv->expander, markup);
			g_free(markup);
		}
	}	
}

static gboolean
dock_update_property_resizable_idle(EinaDock *self)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	EinaDockPrivate *priv = self->priv;

	priv->resizable = gtk_expander_get_expanded(priv->expander);
	g_object_notify((GObject *) self, "resizable");

	return FALSE;
}

static void
dock_reorder_pages(EinaDock *self)
{
	g_return_if_fail(EINA_IS_DOCK(self));
	EinaDockPrivate *priv = self->priv;

	gint pos = 0;
	for (guint i = 0; priv->page_order && priv->page_order[i]; i++)
	{
		GList *link = g_list_find_custom(priv->tabs, priv->page_order[i], (GCompareFunc) list_find_by_id);
		if (link && EINA_IS_DOCK_TAB(link->data))
			gtk_notebook_reorder_child(priv->notebook,
				eina_dock_tab_get_widget((EinaDockTab *) link->data),
				pos++);
	}
}

/*
 * eina_dock_get_resizable:
 *
 * @self: The #EinaDock object
 *
 * Gets the value of the property 'resizable'
 *
 * Returns: the resizable property value
 */
gboolean
eina_dock_get_resizable(EinaDock *self)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), TRUE);
	return self->priv->resizable;
}

/*
 * eina_dock_get_page_order:
 *
 * @self: The #EinaDock object
 *
 * Gets the value of the property 'page-order' as a NULL-terminated array
 * of strings representing the order of the widgets based on they IDs
 *
 * Returns: (element-type utf-8) (transfer full): the page-order property value
 */
gchar **
eina_dock_get_page_order(EinaDock *self)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), NULL);
	return gel_strv_copy(self->priv->page_order, TRUE);
}

/*
 * eina_dock_set_page_order:
 *
 * @self: The #EinaDock object
 * @page_order: (transfer none): A NULL-terminated array of strings representing the order of
 *              widgets based on they IDs
 *
 * Sets the value of the property page-order
 */
void
eina_dock_set_page_order(EinaDock *self, gchar **order)
{	
	g_return_if_fail(EINA_IS_DOCK(self));
	g_return_if_fail(order);

	EinaDockPrivate *priv = self->priv;

	gel_free_and_invalidate(priv->page_order, NULL, g_strfreev);
	priv->page_order = gel_strv_copy(order, TRUE);

	dock_reorder_pages(self);

	g_object_notify((GObject *) self, "page-order");
}

/*
 * eina_dock_add_widget:
 * 
 * @self: The #EinaDock object
 * @id: A string identifying the widget, must be unique
 * @widget: The #GtkWidget to add into the dock
 * @label: (allow-none): Optional label, if NULL a #GtkLabel will be generated.
 *         This widget can be used to represent a shortcut to the item.
 * @flags: #EinaDockFlags to use when adding the widget
 *
 * Adds a new widget to the dock
 *
 * Returns: A #EinaDockTab representing the widget
 */
EinaDockTab *
eina_dock_add_widget(EinaDock *self, const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlags flags)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), NULL);
	gel_warn_fix_implementation();

	EinaDockPrivate *priv = self->priv;

	g_return_val_if_fail(g_list_find_custom(priv->tabs, id, (GCompareFunc) list_find_by_id) == NULL, NULL);

	EinaDockTab *tab = eina_dock_tab_new(id, widget, label, FALSE);
	g_return_val_if_fail(EINA_IS_DOCK_TAB(tab), NULL);

	if (flags != EINA_DOCK_DEFAULT)
		g_warning("EinaDockFlags %x not supported", flags);

	if (priv->n_tabs == 0)
	{
		gtk_box_pack_start   ((GtkBox *) self, widget, FALSE, TRUE, 0);
		gtk_box_reorder_child((GtkBox *) self, widget, 0);
		priv->primary_tab = tab;
	}
	else
	{
		// Get position
		gint pos = 0;
		while (priv->page_order && (priv->page_order[pos] != NULL))
			if (g_str_equal(id, priv->page_order[pos]))
				break;
			else
				pos++;

		// Append notebook's page
		gtk_notebook_append_page  (priv->notebook, widget, label);
		gtk_notebook_reorder_child(priv->notebook, widget, pos);

		// Fix label orientation
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

		gtk_widget_show(label);
	}
	gtk_notebook_set_tab_reorderable(priv->notebook, widget, TRUE);

	// Update structures
	priv->tabs = g_list_prepend(priv->tabs, tab);
	priv->n_tabs++;

	g_object_weak_ref((GObject *) widget, eina_dock_weak_ref_cb, NULL);

	// Update properties
	dock_update_properties(self);
	gtk_widget_show(widget);

	g_signal_emit(self, dock_signals[WIDGET_ADD], 0, id);

	return tab;
}

/*
 * eina_dock_remove_widget:
 * @self: The #EinaDock object.
 * @tab: A #EinaDockTab representing the widget to remove.
 *
 * Removes the widget described by @tab from the #EinaDock object.
 *
 * Returns: %TRUE is operation is completed, %FALSE otherwise.
 */
gboolean
eina_dock_remove_widget(EinaDock *self, EinaDockTab *tab)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(EINA_IS_DOCK_TAB(tab), FALSE);

	EinaDockPrivate *priv = self->priv;

	GList *tab_l = g_list_find_custom(priv->tabs, tab, (GCompareFunc) list_find_by_tab);
	g_return_val_if_fail(tab_l != NULL, FALSE);

	if (tab == priv->primary_tab)
	{
		g_warning(_("Removing primary tab is not supported"));
		return FALSE;
	}

	// Unlink from internal structures
	gtk_notebook_remove_page(
		priv->notebook,
		gtk_notebook_page_num(priv->notebook, eina_dock_tab_get_widget(tab)));

	priv->tabs = g_list_remove_link(priv->tabs, tab_l);
	priv->n_tabs--;
	dock_update_properties(self);

	// Destroy tab
	g_object_weak_unref((GObject *) eina_dock_tab_get_widget(tab), eina_dock_weak_ref_cb, NULL);
	g_object_unref(tab);
	g_list_free(tab_l);

	return TRUE;
}

gboolean
eina_dock_remove_widget_by_id(EinaDock *self, gchar *id)
{
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id, FALSE);

	EinaDockPrivate *priv = self->priv;	

	GList *link = g_list_find_custom(priv->tabs, (gpointer) id, (GCompareFunc) list_find_by_id);
	g_return_val_if_fail((link != NULL) && EINA_IS_DOCK_TAB(link->data), FALSE);

	return eina_dock_remove_widget(self, (EinaDockTab *) link->data);
}

/*
 * eina_dock_switch_widget:
 * @self: The #EinaDock object.
 * @tab: A #EinaDockTab object representing the widget.
 *
 * Forces the dock to show the widget represented by @tab.
 *
 * Returns: %TRUE is successful, %FALSE otherwise
 */
gboolean
eina_dock_switch_widget(EinaDock *self, EinaDockTab *tab)
{
	gel_warn_fix_implementation();
	return FALSE;
	#if 0
	g_return_val_if_fail(EINA_IS_DOCK(self), FALSE);
	g_return_val_if_fail(id != NULL, FALSE);

	EinaDockPrivate *priv = self->priv;

	GtkWidget *dock_item = g_hash_table_lookup(priv->id2tab, (gpointer) id);
	g_return_val_if_fail(dock_item != NULL, FALSE);

	gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(priv->notebook), dock_item);
	g_return_val_if_fail(page_num >= 0, FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
	return TRUE;
	#endif
}

static void
expander_notify_cb(GtkExpander *w, GParamSpec *pspec, EinaDock *self)
{
	if (g_str_equal("expanded", pspec->name))
	{
		g_debug("Expanded is %s", gtk_expander_get_expanded(w) ? "open" : "close");
		g_idle_add((GSourceFunc) dock_update_property_resizable_idle, self);
	}
	else
		g_warning(_("Unhanded notify::%s"), pspec->name);
}

static void
page_reorder_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaDock *self)
{
	gel_warn_fix_implementation();

#if 0
	EinaDockPrivate *priv = self->priv;

	gint n_tabs = gtk_notebook_get_n_pages(w);
	
	gchar **items = g_new0(gchar *, n_tabs + 1);
	gint j = 0;
	for (gint i = 0; i < n_tabs; i++)
	{
		gpointer tab = gtk_notebook_get_nth_page(w, i);

		GHashTableIter iter;
		gchar *id;
		gpointer _tab;
		g_hash_table_iter_init(&iter, priv->id2tab);
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
#endif
}

