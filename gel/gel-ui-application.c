/*
 * gel/gel-ui-application.c
 *
 * Copyright (C) 2004-2010 GelUI
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
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include "gel-ui.h"

G_DEFINE_TYPE (GelUIApplication, gel_ui_application, GTK_TYPE_APPLICATION)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_UI_TYPE_APPLICATION, GelUIApplicationPrivate))

typedef struct _GelUIApplicationPrivate GelUIApplicationPrivate;

struct _GelUIApplicationPrivate {
	GHashTable     *settings;

	GtkWindow      *default_window;
	GtkBox         *container;
	GtkUIManager   *ui_manager;
	GtkActionGroup *ag;
};

static GtkWindow *
create_window(GtkApplication *application);
static void
action_activated_cb(GtkAction *action, GelUIApplication *self);

static gchar *ui_mng_str =
"<ui>"
"  <menubar name='Main' >"
"    <menu name='File' action='FileMenu' >"
"      <menuitem name='Quit' action='QuitItem' />"
"    </menu>"
"    <menu name='Plugins' action='PluginsMenu' >"
"      <menuitem name='PluginManager' action='PluginManagerItem' />"
"    </menu>"
/*
"    <menu name='Edit'    action='EditMenu'    />"
"    <menu name='Help'    action='HelpMenu'    />"
*/
"  </menubar>"
"</ui>";

static GtkActionEntry ui_mng_actions[] = {
	{ "FileMenu",     NULL,           N_("_File"), "<alt>f", NULL, NULL},
		{ "QuitItem", GTK_STOCK_QUIT, NULL,        NULL,     NULL, (GCallback) action_activated_cb },

	{ "PluginsMenu",           NULL, N_("_Add-ons"),        "<alt>a",     NULL, NULL},
		{ "PluginManagerItem", NULL, N_("Select pl_ugins"), "<control>u", NULL,  (GCallback) action_activated_cb }
	/*
	{ "EditMenu",    NULL, N_("_Edit"),    "<alt>e", NULL, NULL},
	{ "HelpMenu",    NULL, N_("_Help"),    "<alt>h", NULL, NULL},
	*/
};

static void
gel_ui_application_dispose (GObject *object)
{
	g_return_if_fail(GEL_UI_IS_APPLICATION(object));

	GelUIApplicationPrivate *priv = GET_PRIVATE((GelUIApplication *) object);
	g_return_if_fail(priv != NULL);

	gel_free_and_invalidate(priv->ag,         NULL, g_object_unref);
	gel_free_and_invalidate(priv->ui_manager, NULL, g_object_unref);
	gel_free_and_invalidate(priv->settings,   NULL, g_hash_table_destroy);

	G_OBJECT_CLASS (gel_ui_application_parent_class)->dispose (object);
}

static void
gel_ui_application_class_init (GelUIApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkApplicationClass *application_class = GTK_APPLICATION_CLASS(klass);

	g_type_class_add_private (klass, sizeof (GelUIApplicationPrivate));

	object_class->dispose = gel_ui_application_dispose;
	application_class->create_window = create_window;
}

static void
gel_ui_application_init (GelUIApplication *self)
{
	GelUIApplicationPrivate *priv = GET_PRIVATE(self);
	priv->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
}

/*
 * gel_ui_application_new:
 *
 * @application_id: (transfer none) (in): An ID for app
 * @argc: (transfer none) (inout): argc
 * @argv: (transfer none) (inout): argv
 *
 * Creates a new #GelUIApplication
 *
 * Returns: (transfer full): the object
 */
GelUIApplication*
gel_ui_application_new (gchar *application_id, gint *argc, gchar ***argv)
{
	gint argc_for_app;
	gchar **argv_for_app;
	GVariant *argv_variant;

	gtk_init (argc, argv);

	if (argc)
		argc_for_app = *argc;
	else
		argc_for_app = 0;

	if (argv)
		argv_for_app = *argv;
	else
		argv_for_app = NULL;

	argv_variant = g_variant_new_bytestring_array ((const gchar * const*) argv_for_app, argc_for_app);

	GError *error = NULL;
	GelUIApplication *self = g_initable_new(GEL_UI_TYPE_APPLICATION, NULL,
		&error,
		"application-id", application_id,
		"argv", argv_variant,
		NULL);

	if (self == NULL)
	{
		g_warning("%s", error->message);
		g_clear_error(&error);
		return NULL;
	}
	return self;
}

static GtkWindow *
create_window(GtkApplication *application)
{
	GelUIApplication *self = GEL_UI_APPLICATION(application);
	GelUIApplicationPrivate *priv = GET_PRIVATE(self);

	if (priv->default_window)
		return priv->default_window;

	priv->default_window = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);

	priv->ag = gtk_action_group_new("_window");
	gtk_action_group_add_actions(priv->ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), self);

	GError *error = NULL;
	priv->ui_manager = gtk_ui_manager_new();
	if ((gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_mng_str, -1 , &error) == 0))
	{
		g_error(N_("Error adding UI string: %s"), error->message);
		g_clear_error(&error);
	}
	gtk_ui_manager_insert_action_group(priv->ui_manager, priv->ag, 0);
	gtk_ui_manager_ensure_update(priv->ui_manager);

	gtk_container_add(
		(GtkContainer *) priv->default_window,
		(GtkWidget *) (priv->container = (GtkBox *) gtk_vbox_new(FALSE, 0))
		);

	gtk_box_pack_start(
		priv->container,
		gtk_ui_manager_get_widget(priv->ui_manager, "/Main"),
		FALSE, TRUE, 0
		);

	gtk_widget_show_all((GtkWidget *) priv->container);
	gtk_window_add_accel_group(priv->default_window, gtk_ui_manager_get_accel_group(priv->ui_manager));

	gchar     *icon_path = NULL;
	GdkPixbuf *icon_pb   = NULL; 
	GError    *err       = NULL;
	if ((icon_path = gel_resource_locate(GEL_RESOURCE_IMAGE, "gel_ui.svg")) &&
	    (icon_pb = gdk_pixbuf_new_from_file_at_size(icon_path, 64, 64, &err)))
	{
		gtk_window_set_default_icon(icon_pb);
		g_object_unref(icon_pb);
	}
	else
	{
		if (!icon_path)
			g_warning(N_("Unable to locate resource '%s'"), "gel_ui.svg");
		else if (!icon_pb)
		{
			g_warning(N_("Unable to load resource '%s': %s"), icon_path, err->message);
			g_error_free(err);
			gel_free_and_invalidate(icon_path, NULL, g_free);
		}
	}

	return priv->default_window;
}

/*
 * gel_ui_application_get_window:
 *
 * @self: the #GelUIApplication
 *
 * Returns: (transfer none): Defaut window for #GelUIApplication
 */
GtkWindow*
gel_ui_application_get_window(GelUIApplication *self)
{
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(self), NULL);
	return gtk_application_create_window((GtkApplication *) self);
}

/*
 * gel_ui_application_get_window_ui_manager:
 *
 * @self: the #GelUIApplication
 *
 * Returns: (transfer none): #GtkUIManager for default window of #GelUIApplication
 */
GtkUIManager*
gel_ui_application_get_window_ui_manager  (GelUIApplication *self)
{
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(self), NULL);
	return GET_PRIVATE(self)->ui_manager;
}

/*
 * gel_ui_application_get_window_action_group:
 *
 * @self: (transfer none) (inout): the #GelUIApplication
 *
 * Returns: (transfer none): #GtkActionGroup for default window of #GelUIApplication
 */
GtkActionGroup*
gel_ui_application_get_window_action_group(GelUIApplication *self)
{
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(self), NULL);
	return GET_PRIVATE(self)->ag;
}

/*
 * gel_ui_application_get_window_content_area:
 *
 * @self: (transfer none) (inout): the #GelUIApplication
 *
 * Returns: (transfer none): #GtkVBox representing content area for default window of #GelUIApplication
 */
GtkVBox*
gel_ui_application_get_window_content_area(GelUIApplication *self)
{
	g_return_val_if_fail(GEL_UI_IS_APPLICATION(self), NULL);
	return GTK_VBOX(GET_PRIVATE(self)->container);
}

/*
 * gel_ui_application_get_settings:
 *
 * @self: (inout) (transfer none): the #GelUIApplication
 * @subdomain: (in) (transfer none): string representing a subdomain
 *
 * Gets a GSettings for the subdomain, if it's not present it gets created.
 * p.ex. Use preferences for org.gnome.test.preferences
 *
 * Returns: (transfer none): The #GSettings object
 */
GSettings*
gel_ui_application_get_settings(GelUIApplication *application, gchar *subdomain)
{
	GelUIApplicationPrivate *priv = GET_PRIVATE(application);

	gchar *app_id = NULL;
	g_object_get(application, "application-id", &app_id, NULL);
	g_return_val_if_fail(app_id != NULL, NULL);

	gchar *domain = g_strconcat(app_id, ".", subdomain, NULL);
	GSettings *ret = g_hash_table_lookup(priv->settings, domain);
	if (ret == NULL)
	{
		ret = g_settings_new(domain);
		g_hash_table_insert(priv->settings, domain, ret);
	}

	return ret;
}

static void
action_activated_cb(GtkAction *action, GelUIApplication *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "QuitItem"))
	{
		gtk_application_quit((GtkApplication *) self);
		return;
	}

	g_warning(N_("Ignoring unknow action '%s'"), name);
}


