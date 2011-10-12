/*
 * eina/dock/eina-dock-tab.c
 *
 * Copyright (C) 2004-2011 Eina
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

/**
 * SECTION:eina-dock-tab
 * @title: EinaDockTab
 * @short_description: Tab management
 *
 * #EinaDockTab represents in an abstract way a #EinaDock tab. It is not
 * a #GtkWidget but a #GObject, functions like eina_dock_tab_get_widget()
 * or eina_dock_tab_get_label() are the way to get internal widgets.
 */

#include "eina-dock-tab.h"

G_DEFINE_TYPE (EinaDockTab, eina_dock_tab, G_TYPE_OBJECT)

struct _EinaDockTabPrivate {
	gchar     *id;
	GtkWidget *widget, *label;
	gboolean   primary;
};

enum {
	PROP_0,
	PROP_ID,
	PROP_WIDGET,
	PROP_LABEL,
	PROP_PRIMARY
};

static void dock_tab_set_id    (EinaDockTab *self, const gchar *id);
static void dock_tab_set_widget(EinaDockTab *self, GtkWidget *widget);
static void dock_tab_set_label (EinaDockTab *self, GtkWidget *label);

static void
eina_dock_tab_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	EinaDockTab *self = EINA_DOCK_TAB(object);

	switch (property_id) {

	case PROP_ID:
		g_value_set_string(value, eina_dock_tab_get_id(self));
		break;

	case PROP_WIDGET:
		g_value_set_object(value, eina_dock_tab_get_widget(self));
		break;

	case PROP_LABEL:
		g_value_set_object(value, eina_dock_tab_get_label(self));
		break;

	case PROP_PRIMARY:
		g_value_set_boolean(value, eina_dock_tab_get_primary(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_dock_tab_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	EinaDockTab *self = EINA_DOCK_TAB(object);

	switch (property_id) {

	case PROP_ID:
		dock_tab_set_id(self, g_value_get_string(value));
		break;

	case PROP_WIDGET:
		dock_tab_set_widget(self, (GtkWidget *) g_value_get_object(value));
		break;

	case PROP_LABEL:
		dock_tab_set_label(self, (GtkWidget *) g_value_get_object(value));
		break;



	case PROP_PRIMARY:
		eina_dock_tab_set_primary(self, g_value_get_boolean(value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_dock_tab_dispose (GObject *object)
{
	EinaDockTab *self = EINA_DOCK_TAB(object);
	EinaDockTabPrivate *priv = self->priv;

	if (priv->id)
	{
		g_free(priv->id);
		priv->id = NULL;
	}

	if (priv->widget)
	{
		g_object_unref(priv->widget);
		priv->widget = NULL;
	}

	if (priv->label)
	{
		g_object_unref(priv->label);
		priv->label = NULL;
	}

	G_OBJECT_CLASS (eina_dock_tab_parent_class)->dispose (object);
}

static void
eina_dock_tab_class_init (EinaDockTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaDockTabPrivate));

	object_class->get_property = eina_dock_tab_get_property;
	object_class->set_property = eina_dock_tab_set_property;
	object_class->dispose = eina_dock_tab_dispose;

	/**
	 * EinaDockTab:id:
	 *
	 * Unique ID for docktab
	 */
	g_object_class_install_property(object_class, PROP_ID,
		g_param_spec_string("id", "id", "id",
		NULL, G_PARAM_STATIC_STRINGS|G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
	/**
	 * EinaDockTab:widget:
	 *
	 * Main widget for docktab
	 */
	g_object_class_install_property(object_class, PROP_WIDGET,
		g_param_spec_object("widget", "widget", "widget",
		GTK_TYPE_WIDGET, G_PARAM_STATIC_STRINGS|G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
	/**
	 * EinaDockTab:label:
	 *
	 * Label widget for docktab
	 */
	g_object_class_install_property(object_class, PROP_LABEL,
		g_param_spec_object("label", "label", "label",
		GTK_TYPE_WIDGET, G_PARAM_STATIC_STRINGS|G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
	/**
	 * EinaDockTab:primary:
	 *
	 * Whatever the tab is primary or not
	 */
	g_object_class_install_property(object_class, PROP_PRIMARY,
		g_param_spec_boolean("primary", "primary", "primary",
		FALSE, G_PARAM_STATIC_STRINGS|G_PARAM_READWRITE));
}

static void
eina_dock_tab_init (EinaDockTab *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_DOCK_TAB, EinaDockTabPrivate));
}

/**
 * eina_dock_tab_new:
 * @id: Unique ID for the tab. NOT checked ATM
 * @widget: (transfer none): Main widget for the tab
 * @label: (transfer none): Label widget for the tab
 * @primary: Whatever is primary or not
 *
 * Creates a new #EinaDockTab
 *
 * Returns: (type Eina.DockTab) (transfer full): The tab
 */
EinaDockTab*
eina_dock_tab_new (const gchar *id, GtkWidget *widget, GtkWidget *label, gboolean primary)
{
	return g_object_new (EINA_TYPE_DOCK_TAB,
		"id", id,
		"widget", widget,
		"label", label,
		"primary", primary,
		NULL);
}

/**
 * eina_dock_tab_get_id:
 * @self: An #EinaDockTab
 *
 * Gets the value of EinaDockTab:id property
 *
 * Returns: The Unique ID
 */
const gchar*
eina_dock_tab_get_id(EinaDockTab *self)
{
	g_return_val_if_fail(EINA_IS_DOCK_TAB(self), NULL);
	return self->priv->id;
}

static void
dock_tab_set_id(EinaDockTab *self, const gchar *id)
{
	g_return_if_fail(EINA_IS_DOCK_TAB(self));
	g_return_if_fail(id != NULL);

	g_return_if_fail(self->priv->id == NULL);

	self->priv->id = g_strdup(id);

	g_object_notify((GObject *) self, "id");
}

/**
 * eina_dock_tab_get_widget:
 * @self: An #EinaDockTab
 *
 * Gets the value of EinaDockTab:widget property
 *
 * Returns: (transfer none): The main widget
 */
GtkWidget*
eina_dock_tab_get_widget(EinaDockTab *self)
{
	g_return_val_if_fail(EINA_IS_DOCK_TAB(self), NULL);
	return self->priv->widget;
}

static void
dock_tab_set_widget(EinaDockTab *self, GtkWidget *widget)
{
	g_return_if_fail(EINA_IS_DOCK_TAB(self));
	g_return_if_fail(GTK_IS_WIDGET(widget));

	g_return_if_fail(self->priv->widget == NULL);

	self->priv->widget = g_object_ref(widget);

	g_object_notify((GObject *) self, "widget");
}

/**
 * eina_dock_tab_get_label:
 * @self: An #EinaDockTab
 *
 * Gets the value of EinaDockTab:label property
 *
 * Returns: (transfer none): The label widget
 */
GtkWidget*
eina_dock_tab_get_label(EinaDockTab *self)
{
	g_return_val_if_fail(EINA_IS_DOCK_TAB(self), NULL);
	return self->priv->label;
}

static void
dock_tab_set_label(EinaDockTab *self, GtkWidget *label)
{
	g_return_if_fail(EINA_IS_DOCK_TAB(self));
	g_return_if_fail(GTK_IS_WIDGET(label));

	g_return_if_fail(self->priv->label == NULL);

	self->priv->label = g_object_ref(label);

	g_object_notify((GObject *) self, "label");
}

/**
 * eina_dock_tab_get_primary:
 * @self: An #EinaDockTab
 *
 * Gets the value of EinaDockTab:primary property
 *
 * Returns: Whatever @self is primary or not
 */
gboolean
eina_dock_tab_get_primary(EinaDockTab *self)
{
	g_return_val_if_fail(EINA_IS_DOCK_TAB(self), FALSE);
	return self->priv->primary;
}

/**
 * eina_dock_tab_set_primary:
 * @self: An #EinaDockTab
 * @primary: Value for EinaDockTab:primary property
 *
 * Sets the value of EinaDockTab:primary
 */
void
eina_dock_tab_set_primary(EinaDockTab *self, gboolean primary)
{
	g_return_if_fail(EINA_IS_DOCK_TAB(self));

	if (primary == self->priv->primary)
		return;
	self->priv->primary = primary;
	g_object_notify((GObject *) self, "primary");
}

/**
 * eina_dock_tab_equal:
 * @a: (transfer none): An #EinaDockTab
 * @b: (transfer none): An #EinaDockTab
 *
 * Compares two EinaDockTab for equally, this means id and widget are equal
 *
 * Returns: The equality of @a and @b
 */
gboolean
eina_dock_tab_equal(EinaDockTab *a, EinaDockTab *b)
{
	g_return_val_if_fail(EINA_IS_DOCK_TAB(a), FALSE);
	g_return_val_if_fail(EINA_IS_DOCK_TAB(b), FALSE);

	return (g_str_equal(eina_dock_tab_get_id(a), eina_dock_tab_get_id(b)) &&
		(eina_dock_tab_get_widget(a) == eina_dock_tab_get_widget(b)));
}

