/*
 * eina/ext/eina-window.c
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
 * SECTION:eina-window
 * @title: EinaWindow
 * @short_description: Eina main window widget
 *
 * #EinaWindow is the widget used for the main Eina's window.
 * It's a regular #GtkWindow plus a #GtkUIManager and the associated
 * #GtkActionGroup. Aditionaly #EinaWindow can be persistant, this is the
 * ability to automatically do hide-on-delete.
 *
 * If you want to add menus to main window use eina_window_get_ui_manager() and
 * eina_window_get_action_group(). For persistance features see
 * #EinaWindow:persistant property
 *
 * See also: #GtkWindow
 */

#include "eina-window.h"
#include <glib/gi18n.h>
#include <libpeas-gtk/peas-gtk.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-stock.h>

G_DEFINE_TYPE (EinaWindow, eina_window, GTK_TYPE_WINDOW)

struct _EinaWindowPrivate {
	GtkBox    *container;

	GtkActionGroup *ag;
	GtkUIManager   *ui_manager;

	GtkDialog *plugin_dialog;
	gulong   persistant_handler_id;
};

static gboolean
plugin_dialog_cleanup(EinaWindow *self);
static void
action_activated_cb(GtkAction *action, EinaWindow *self);

static gchar *ui_mng_str =
"<ui>"
"  <menubar name='Main' >"
"    <menu name='File' action='file-menu' >"
"      <menuitem name='Quit' action='quit-action' />"
"    </menu>"
"    <menu name='Edit' action='edit-menu' >"
"      <menuitem name='Addons' action='plugins-action' />"
"    </menu>"
"    <menu name='Tools' action='plugins-menu' >"
"    </menu>"
"    <menu name='Help' action='help-menu' >"
"    </menu>"
"  </menubar>"
"</ui>";

static GtkActionEntry ui_mng_actions[] = {
	{ "file-menu",    NULL,           N_("_File"),    "<alt>f", NULL, NULL},
	{ "edit-menu",    NULL,           N_("_Edit"),    "<alt>e", NULL, NULL},
	{ "plugins-menu", NULL,           N_("_Plugins"), "<alt>p", NULL, NULL},
	{ "help-menu",    NULL,           N_("_Help"),    "<alt>h", NULL, NULL},

	{ "plugins-action", EINA_STOCK_PLUGIN, N_("_Plugins"), "<control>u", NULL, (GCallback) action_activated_cb },
	{ "quit-action",    GTK_STOCK_QUIT,    NULL,           NULL,         NULL, (GCallback) action_activated_cb }
};

enum {
	PROP_PERSISTANT = 1
};

enum {
	ACTION_ACTIVATE,
	LAST_SIGNAL
};
guint window_signals[LAST_SIGNAL] = { 0 };

static void
eina_window_container_add(GtkContainer *container, GtkWidget *widget)
{
	EinaWindow *self = EINA_WINDOW(container);
	guint n_children = g_list_length(gtk_container_get_children(GTK_CONTAINER(self->priv->container)));

	g_return_if_fail(n_children <= 2);
	gtk_box_pack_start(self->priv->container, widget, TRUE, TRUE, 0);
}

static void
eina_window_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PERSISTANT:
		eina_window_set_persistant(EINA_WINDOW(object), g_value_get_boolean(value));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_window_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_PERSISTANT:
		g_value_set_boolean(value, eina_window_get_persistant(EINA_WINDOW(object)));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_window_dispose (GObject *object)
{
	EinaWindow *self = EINA_WINDOW(object);

	plugin_dialog_cleanup(self);
	gel_free_and_invalidate(self->priv->ag,            NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->ui_manager,    NULL, g_object_unref);
	G_OBJECT_CLASS (eina_window_parent_class)->dispose (object);
}

static void
eina_window_class_init (EinaWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaWindowPrivate));

	object_class->set_property = eina_window_set_property;
	object_class->get_property = eina_window_get_property;
	object_class->dispose = eina_window_dispose;

	/**
	 * EinaWindow:persistant:
	 *
	 * Tell window if must be hide on delete-event or just hide
	 */
	g_object_class_install_property(object_class, PROP_PERSISTANT,
		g_param_spec_boolean("persistant", "persistant", "persistant",
			FALSE, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

	/**
	 * EinaWindow::action-activate:
	 * @window: The #EinaWindow
	 * @action: #GtkAction that has been activated
	 *
	 * Emitted if some action is activated
	 *
	 * Returns: %TRUE if action was handed by callback and stop processing,
	 * %FALSE if default handler need to be called.
	 */
	window_signals[ACTION_ACTIVATE] =
		g_signal_new ("action-activate",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaWindowClass, action_activate),
		NULL, NULL,
		gel_marshal_BOOLEAN__OBJECT,
		G_TYPE_BOOLEAN,
		1,
		G_TYPE_OBJECT);

}

static void
eina_window_init (EinaWindow *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_WINDOW, EinaWindowPrivate));

	self->priv->ag = gtk_action_group_new("_window");
	gtk_action_group_add_actions(self->priv->ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), self);

	GError *error = NULL;
	self->priv->ui_manager = gtk_ui_manager_new();
	if ((gtk_ui_manager_add_ui_from_string (self->priv->ui_manager, ui_mng_str, -1 , &error) == 0))
	{
		g_error(N_("Error adding UI string: %s"), error->message);
		g_clear_error(&error);
	}
	gtk_ui_manager_insert_action_group(self->priv->ui_manager, self->priv->ag, 0);
	gtk_ui_manager_ensure_update(self->priv->ui_manager);

	GtkBox *tmpbox = (GtkBox *) gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(tmpbox, FALSE);

	gtk_container_add(
		(GtkContainer *) self,
		(GtkWidget *) tmpbox
		);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS(G_OBJECT_GET_CLASS(self));
	container_class->add = eina_window_container_add;

	gtk_box_pack_start(
		tmpbox,
		gtk_ui_manager_get_widget(self->priv->ui_manager, "/Main"),
		FALSE, TRUE, 0
		);
	gtk_box_pack_start(
		tmpbox,
		(GtkWidget *) (self->priv->container = (GtkBox *) gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),
		TRUE, TRUE, 0
		);
		gtk_box_set_homogeneous((GtkBox *) self->priv->container, FALSE);

	gtk_widget_show_all((GtkWidget *) tmpbox);
	gtk_window_add_accel_group((GtkWindow *) self, gtk_ui_manager_get_accel_group(self->priv->ui_manager));
}

/**
 * eina_window_new:
 *
 * Creates a new #EinaWindow
 *
 * Returns: (transfer full): The #EinaWindow
 */
EinaWindow*
eina_window_new (void)
{
	return g_object_new (EINA_TYPE_WINDOW, NULL);
}

/**
 * eina_window_get_ui_manager:
 * @self: the #EinaWindow
 *
 * Gets the #GtkUIManager for the main window
 *
 * Returns: (transfer none): A #GtkUIManager
 */
GtkUIManager*
eina_window_get_ui_manager(EinaWindow *self)
{
	g_return_val_if_fail(EINA_IS_WINDOW(self), NULL);
	return GTK_UI_MANAGER(self->priv->ui_manager);
}

/**
 * eina_window_get_action_group:
 * @self: the #EinaWindow
 *
 * Gets the #GtkActionGroup for the main window
 *
 * Returns: (transfer none): #GtkActionGroup for default window of #EinaWindow
 */
GtkActionGroup*
eina_window_get_action_group(EinaWindow *self)
{
	g_return_val_if_fail(EINA_IS_WINDOW(self), NULL);
	return GTK_ACTION_GROUP(self->priv->ag);
}

/**
 * eina_window_set_persistant:
 * @self: An #EinaWindow
 * @persistant: Value for the 'persistant' property
 *
 * Sets the #EinaWindow:persistant property
 */
void
eina_window_set_persistant(EinaWindow *self, gboolean persistant)
{
	g_return_if_fail(EINA_IS_WINDOW(self));
	if ((persistant && self->priv->persistant_handler_id)
	    || (!persistant && !self->priv->persistant_handler_id))
		return;

	if (persistant)
	{
		g_return_if_fail(self->priv->persistant_handler_id == 0);
		self->priv->persistant_handler_id = g_signal_connect(self, "delete-event", (GCallback) gtk_widget_hide_on_delete, NULL);
	}
	else
	{
		g_return_if_fail(self->priv->persistant_handler_id > 0);
		g_signal_handler_disconnect(self,  self->priv->persistant_handler_id);
		self->priv->persistant_handler_id = 0;
	}

	g_object_notify(G_OBJECT(self), "persistant");
}

/**
 * eina_window_get_persistant:
 * @self: An #EinaWindow
 *
 * Gets the ::persistant property
 *
 * Returns: Value for the ::persistant property
 */
gboolean
eina_window_get_persistant(EinaWindow *self)
{
	g_return_val_if_fail(EINA_IS_WINDOW(self), FALSE);
	return (self->priv->persistant_handler_id > 0);
}

static gboolean
plugin_dialog_cleanup(EinaWindow *self)
{
	g_return_val_if_fail(EINA_IS_WINDOW(self), FALSE);
	if (!self->priv->plugin_dialog)
		return FALSE;

	gtk_widget_destroy((GtkWidget *) self->priv->plugin_dialog);
	peas_engine_garbage_collect(peas_engine_get_default());
	self->priv->plugin_dialog = NULL;

	return FALSE;
}

static void
action_activated_cb(GtkAction *action, EinaWindow *self)
{
	g_return_if_fail(EINA_IS_WINDOW(self));
	g_return_if_fail(GTK_IS_ACTION(action));

	gboolean ret = FALSE;
	g_signal_emit(self, window_signals[ACTION_ACTIVATE], 0, action, &ret);

	if (ret)
		return;

	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "quit-action"))
	{
		GtkApplication *app = gtk_window_get_application((GtkWindow *) self);
		gtk_application_remove_window(app, (GtkWindow *) self);
		g_application_release(G_APPLICATION(app));
		return;
	}

	else if (g_str_equal(name, "plugins-action"))
	{
		if (!self->priv->plugin_dialog)
		{
			self->priv->plugin_dialog = (GtkDialog *) gtk_dialog_new_with_buttons(_("Select plugins"),
				(GtkWindow *) self,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);

			GtkWidget *container = (GtkWidget *) gtk_dialog_get_content_area(self->priv->plugin_dialog);
			g_object_set((GObject *) container, "margin", 12, NULL);

			GtkWidget *pm = (GtkWidget *) peas_gtk_plugin_manager_new(NULL);
			gtk_box_pack_start((GtkBox*) container, pm, TRUE, TRUE, 0);
			gtk_widget_show_all(pm);

			g_signal_connect_swapped(self->priv->plugin_dialog, "destroy",  (GCallback) plugin_dialog_cleanup, self);
			g_signal_connect_swapped(self->priv->plugin_dialog, "response", (GCallback) plugin_dialog_cleanup, self);
		}

		gtk_window_present((GtkWindow *) self->priv->plugin_dialog);
		return;
	}

	g_warning(N_("Unhandled unknow action '%s'"), name);
}

