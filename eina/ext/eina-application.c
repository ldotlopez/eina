/*
 * eina/ext/eina-application.c
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
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <libpeas/peas-engine.h>
#include <libpeasui/peas-ui-plugin-manager.h>
#include <gel/gel.h>
#include "eina-application.h"

G_DEFINE_TYPE (EinaApplication, eina_application, GTK_TYPE_APPLICATION)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_APPLICATION, EinaApplicationPrivate))

#define EINA_APPLICATION_DOMAIN "net.sourceforge.eina"

typedef struct _EinaApplicationPrivate EinaApplicationPrivate;

struct _EinaApplicationPrivate {
	GtkWindow      *default_window;
	GtkBox         *container;
	GtkUIManager   *ui_manager;
	GtkActionGroup *ag;

	GHashTable *settings, *shared;

	PeasEngine *engine;
};

static GtkWindow *
create_window(GtkApplication *application);
static void
action_activated_cb(GtkAction *action, EinaApplication *self);
static void
engine_load_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *self);
static void
engine_unload_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *self);

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
eina_application_dispose (GObject *object)
{
	g_return_if_fail(EINA_IS_APPLICATION(object));

	EinaApplicationPrivate *priv = GET_PRIVATE((EinaApplication *) object);
	g_return_if_fail(priv != NULL);

	gel_free_and_invalidate(priv->engine,     NULL, g_object_unref);
	
	gel_free_and_invalidate(priv->ag,         NULL, g_object_unref);
	gel_free_and_invalidate(priv->ui_manager, NULL, g_object_unref);
	gel_free_and_invalidate(priv->settings,   NULL, g_hash_table_destroy);
	gel_free_and_invalidate(priv->shared,     NULL, g_hash_table_destroy);

	G_OBJECT_CLASS (eina_application_parent_class)->dispose (object);
}

static void
eina_application_class_init (EinaApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkApplicationClass *application_class = GTK_APPLICATION_CLASS(klass);

	g_type_class_add_private (klass, sizeof (EinaApplicationPrivate));

	object_class->dispose = eina_application_dispose;
	application_class->create_window = create_window;
}

static void
eina_application_init (EinaApplication *self)
{
	EinaApplicationPrivate *priv = GET_PRIVATE(self);
	priv->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	priv->shared   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	gchar const * const search_paths[] = {
		/* Uninstalled plugins */
		"./plugins/",
		"./plugins/",
		NULL };

	priv->engine = peas_engine_new("Eina", NULL, (const gchar **) search_paths);

	g_signal_connect(priv->engine, "load-plugin",   (GCallback) engine_load_plugin_cb,   self);
	g_signal_connect(priv->engine, "unload-plugin", (GCallback) engine_unload_plugin_cb, self);
}

static GVariant *
variant_from_argv (int argc, char **argv)
{
	GVariantBuilder builder;
	int i;

	g_variant_builder_init (&builder, G_VARIANT_TYPE ("aay"));

	for (i = 1; i < argc; i++)
	{
		guint8 *argv_bytes;

		argv_bytes = (guint8*) argv[i];
		g_variant_builder_add_value (&builder,
		g_variant_new_byte_array (argv_bytes, -1));
	}

	return g_variant_builder_end (&builder);
}

EinaApplication*
eina_application_new (gint *argc, gchar ***argv)
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

	argv_variant = variant_from_argv (argc_for_app, argv_for_app);


	GError *error = NULL;
	EinaApplication *self = g_initable_new(EINA_TYPE_APPLICATION, NULL,
		&error,
		"application-id", EINA_APPLICATION_DOMAIN,
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
	EinaApplication *self = EINA_APPLICATION(application);
	EinaApplicationPrivate *priv = GET_PRIVATE(self);

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
	if ((icon_path = gel_resource_locate(GEL_RESOURCE_IMAGE, "eina.svg")) &&
	    (icon_pb = gdk_pixbuf_new_from_file_at_size(icon_path, 64, 64, &err)))
	{
		gtk_window_set_default_icon(icon_pb);
		g_object_unref(icon_pb);
	}
	else
	{
		if (!icon_path)
			g_warning(N_("Unable to locate resource '%s'"), "eina.svg");
		else if (!icon_pb)
		{
			g_warning(N_("Unable to load resource '%s': %s"), icon_path, err->message);
			g_error_free(err);
			gel_free_and_invalidate(icon_path, NULL, g_free);
		}
	}

	return priv->default_window;
}

GtkWindow*
eina_application_get_window(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return gtk_application_create_window((GtkApplication *) self);
}

GtkUIManager*
eina_application_get_window_ui_manager  (EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return GET_PRIVATE(self)->ui_manager;
}

GtkActionGroup*
eina_application_get_window_action_group(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return GET_PRIVATE(self)->ag;
}

GtkVBox*
eina_application_get_window_content_area(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return GTK_VBOX(GET_PRIVATE(self)->container);
}

GSettings*
eina_application_get_settings(EinaApplication *application, gchar *subdomain)
{
	EinaApplicationPrivate *priv = GET_PRIVATE(application);

	gchar *domain = g_strconcat(EINA_APPLICATION_DOMAIN, ".", subdomain, NULL);
	GSettings *ret = g_hash_table_lookup(priv->settings, domain);
	if (ret == NULL)
	{
		ret = g_settings_new(domain);
		g_hash_table_insert(priv->settings, domain, ret);
	}
	g_free(domain);

	return ret;
}

gboolean
eina_application_set_shared(EinaApplication *self, gchar *key, gpointer data)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), FALSE);
	EinaApplicationPrivate *priv = GET_PRIVATE(self);

	g_return_val_if_fail(key != NULL, FALSE);
	g_return_val_if_fail(g_hash_table_lookup(priv->shared, key) == NULL, FALSE);

	g_hash_table_insert(priv->shared, g_strdup(key), data);
	return TRUE;
}

gpointer
eina_application_get_shared(EinaApplication *self, gchar *key)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	g_return_val_if_fail(key != NULL, NULL);

	gpointer ret = g_hash_table_lookup(GET_PRIVATE(self)->shared, key);
	g_return_val_if_fail(ret != NULL, NULL);

	return ret;
}

static void
action_activated_cb(GtkAction *action, EinaApplication *self)
{
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "QuitItem"))
	{
		gtk_application_quit((GtkApplication *) self);
		return;
	}

	if (g_str_equal(name, "PluginManagerItem"))
	{
		GtkDialog *dialog = (GtkDialog *) gtk_dialog_new();
		gtk_container_add(
			(GtkContainer *) gtk_dialog_get_content_area(dialog),
			peas_ui_plugin_manager_new(GET_PRIVATE(self)->engine));
		gtk_widget_show_all((GtkWidget *) dialog);
		gtk_dialog_run(dialog);
		return;
	}

	g_warning(N_("Ignoring unknow action '%s'"), name);
}

static void
engine_load_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *self)
{
	g_warning("Loaded plugin: %s", peas_plugin_info_get_name(info));
}

static void
engine_unload_plugin_cb(PeasEngine *engine, PeasPluginInfo *info, EinaApplication *self)
{
	g_warning("Unloaded plugin: %s", peas_plugin_info_get_name(info));
}

