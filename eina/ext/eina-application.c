/*
 * eina/ext/eina-application.c
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
 * SECTION:eina-application
 * @title: EinaApplication
 * @short_description: Application abstraction for Eina
 *
 * #EinaApplication is the central point or the hub for Eina. Every component is
 * accesible from #EinaApplication via the eina_application_get_interface()
 * call. Other functions for access main window interface, launch external
 * programs or single instance model are provides by this class.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEBUG 1
#define DEBUG_PREFIX "EinaApplication"
#if DEBUG
#define debug(...) g_debug(DEBUG_PREFIX " " __VA_ARGS__)
#else
#define debug(...) ;
#endif

#include "eina-application.h"
#include <gel/gel.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>
#include <eina/ext/eina-activatable.h>
#include <eina/ext/eina-window.h>

G_DEFINE_TYPE (EinaApplication, eina_application, GTK_TYPE_APPLICATION)

struct _EinaApplicationPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *interfaces;
	GHashTable *settings;

	EinaWindow *window;
};

static void
eina_application_startup(GApplication *application);
static void
create_window(EinaApplication *self);
static void
activate_cb(EinaApplication *self);

static gboolean
window_delete_event_cb(EinaWindow *window, GdkEvent *ev, EinaApplication *self);

static PeasEngine*
application_get_plugin_engine(EinaApplication *app)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(app), NULL);
	return (PeasEngine *) g_object_get_data((GObject *) app, "x-eina-plugin-engine");
}

static void
application_set_plugin_engine(EinaApplication *app, PeasEngine *engine)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(PEAS_IS_ENGINE(engine));

	PeasEngine *test = application_get_plugin_engine(app);
	if (test != NULL)
		g_warning("EinaApplication object %p already has an PeasEngine", test);
	g_object_set_data((GObject *) app, "x-eina-plugin-engine", engine);
}

static void
extension_set_extension_added_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, EinaApplication *application)
{
	GError *error = NULL;
	if (!eina_activatable_activate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to activate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
}

static void
extension_set_extension_removed_cb(PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *exten, EinaApplication *application)
{
	GError *error = NULL;
	if (!eina_activatable_deactivate(EINA_ACTIVATABLE (exten), application, &error))
	{
		g_warning(_("Unable to deactivate plugin %s: %s"), peas_plugin_info_get_name(info), error->message);
		g_error_free(error);
	}
}

/**
 * eina_application_create_standalone_engine:
 * @from_source: %TRUE if this code is run from source dir. Most of the time
 * this should be %FALSE
 *
 * Creates a new plugin engine
 *
 * Returns: (transfer full): A #PeasEngine
 */
PeasEngine*
eina_application_create_standalone_engine(gboolean from_source)
{
	gchar *builddir = NULL;
	if (from_source)
	{
		g_type_init();
		gel_init(PACKAGE, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR);
	}

	// Setup girepository
	if (from_source)
	{
		builddir = g_path_get_dirname(g_get_current_dir());
		const gchar *subs[] = { "gel", "lomo", "eina", NULL };
		for (guint i = 0; subs[i]; i++)
		{
			gchar *tmp = g_build_filename(builddir, subs[i], NULL);
			g_debug("Add girepository search path: %s", tmp);
			g_irepository_prepend_search_path(tmp);
			g_free(tmp);
		}
	}

	const gchar *g_ir_req_full[] = { PACKAGE_NAME, EINA_API_VERSION, NULL };
	const gchar *g_ir_req_src[]  = { "Gel", GEL_API_VERSION, "Lomo", LOMO_API_VERSION, NULL };
	const gchar **g_ir_reqs = (const gchar **) (from_source ? g_ir_req_src : g_ir_req_full);

	GError *error = NULL;
	GIRepository *repo = g_irepository_get_default();
	for (guint i = 0; g_ir_reqs[i] != NULL; i = i + 2)
	{
		if (!g_irepository_require(repo, g_ir_reqs[i], g_ir_reqs[i+1], G_IREPOSITORY_LOAD_FLAG_LAZY, &error))
		{
			g_warning(N_("Unable to load typelib %s %s: %s"), g_ir_reqs[i], g_ir_reqs[i+1], error->message);
			g_error_free(error);
			return NULL;
		}
	}

	PeasEngine *engine = peas_engine_get_default();
	peas_engine_enable_loader (engine, "python");

	if (from_source)
	{
		gchar *plugins[] = { "lomo", "preferences", "dock" , "playlist", "player",
			#if HAVE_GTKMAC
			"osx",
			#endif
			#if HAVE_SQLITE3
			"adb", "muine",
			#endif
			NULL };
		for (guint i = 0; plugins[i]; i++)
		{
			gchar *tmp = g_build_filename(builddir, "eina", plugins[i], NULL);
			g_debug("Add PeasEngine search path: %s", tmp);
			peas_engine_add_search_path(engine, tmp, tmp);
			g_free(tmp);
		}
	}
	else
	{
		const gchar *libdir = NULL;

		if ((libdir = gel_get_package_lib_dir()) != NULL)
			peas_engine_add_search_path(engine, gel_get_package_lib_dir(), gel_get_package_lib_dir());

		if ((libdir = g_getenv("EINA_LIB_PATH")) != NULL)
			peas_engine_add_search_path(engine, g_getenv("EINA_LIB_PATH"), g_getenv("EINA_LIB_PATH"));

		gchar *user_plugin_path = NULL;

		#if defined OS_LINUX
		user_plugin_path = g_build_filename(g_get_user_data_dir(),
			PACKAGE, "plugins",
			NULL);

		#elif defined OS_OSX
		user_plugin_path = g_build_filename(g_get_home_dir(),
			"Library", "Application Support",
			PACKAGE_NAME, "Plugins",
			NULL);
		#endif

		if (user_plugin_path)
		{
			peas_engine_add_search_path(engine, user_plugin_path, user_plugin_path);
			g_free(user_plugin_path);
		}
	}

	gel_free_and_invalidate(builddir, NULL, g_free);

	return engine;
}

static void
eina_application_dispose (GObject *object)
{
	EinaApplication *self = EINA_APPLICATION(object);

	gel_free_and_invalidate(self->priv->interfaces, NULL, g_hash_table_destroy);
	if (self->priv->settings)
	{
		GList *values = g_hash_table_get_values(self->priv->settings);
		g_list_foreach(values, (GFunc) g_settings_sync, NULL);
		g_list_free(values);
		gel_free_and_invalidate(self->priv->settings,   NULL, g_hash_table_destroy);
	}

	G_OBJECT_CLASS (eina_application_parent_class)->dispose (object);
}

static void
eina_application_class_init (EinaApplicationClass *klass)
{
	GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
	GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaApplicationPrivate));

	object_class->dispose = eina_application_dispose;
	application_class->startup    = eina_application_startup;
}

static void
eina_application_init (EinaApplication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_APPLICATION, EinaApplicationPrivate);
	self->priv->interfaces = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	self->priv->settings   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
}

static void
eina_application_startup(GApplication *application)
{
	EinaApplication *self = (EinaApplication *)application;
	g_return_if_fail(EINA_IS_APPLICATION(self));

	G_APPLICATION_CLASS (eina_application_parent_class)->startup(G_APPLICATION(self));

	// Generate iconlist for windows
	const gint sizes[] = { 16, 32, 48, 64, 128 };
	GList *icon_list = NULL;
	gchar *icon_filename = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "eina.svg");
	if (icon_filename)
	{
		GError *e = NULL;
		GdkPixbuf *pb = NULL;
		for (guint i = 0; i < G_N_ELEMENTS(sizes); i++)
		{
			if (!(pb = gdk_pixbuf_new_from_file_at_scale(icon_filename, sizes[i], sizes[i], TRUE, &e)))
			{
				g_warning(N_("Unable to load resource '%s': %s"), icon_filename, e->message);
				break;
			}
			else
				icon_list = g_list_prepend(icon_list, pb);
		}
		gel_free_and_invalidate(e, NULL, g_error_free);

		gtk_window_set_default_icon_list(icon_list);
		gel_list_deep_free(icon_list, g_object_unref);
	}
	else
		g_warning(N_("Unable to locate resource '%s'"), "eina.svg");

	// Create main window
	create_window(self);

	// Setup engine
	PeasEngine *engine = eina_application_create_standalone_engine(FALSE);
	application_set_plugin_engine(EINA_APPLICATION(application), engine);

	// ExtensionSet
	PeasExtensionSet *es = peas_extension_set_new (engine,
		EINA_TYPE_ACTIVATABLE,
		"application", application,
		NULL);
	peas_extension_set_call(es, "activate", application, NULL);

	g_signal_connect(es, "extension-added",   G_CALLBACK(extension_set_extension_added_cb),   application);
	g_signal_connect(es, "extension-removed", G_CALLBACK(extension_set_extension_removed_cb), application);

	// Load plugins
	gchar  *req_plugins[] = { "dbus", "player", "playlist",
		#if HAVE_GTKMAC
		"osx",
		#endif
		NULL };

	gchar **opt_plugins = g_settings_get_strv(
			eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN),
			"plugins");

	gchar **plugins = gel_strv_concat(
		req_plugins,
		opt_plugins,
		NULL);
	g_strfreev(opt_plugins);

	guint  n_req_plugins = g_strv_length(req_plugins);
	guint  n_plugins = g_strv_length(plugins);
	guint  i;
	for (i = 0; i < n_plugins; i++)
	{
		PeasPluginInfo *info = peas_engine_get_plugin_info(engine, plugins[i]);
		if (!info)
			g_warning(_("Unable to load plugin info"));

		// If plugin is hidden and is optional dont load.
		if (peas_plugin_info_is_hidden(info) && (i >= n_req_plugins))
			continue;

		if (!peas_engine_load_plugin(engine, info))
			g_warning(N_("Unable to load required plugin '%s'"), plugins[i]);
	}
	g_strfreev(plugins);

	g_settings_bind (
		eina_application_get_settings(EINA_APPLICATION(application), EINA_DOMAIN), "plugins",
		engine, "loaded-plugins",
		G_SETTINGS_BIND_SET);
	gtk_widget_show((GtkWidget *) eina_application_get_window((EinaApplication *) application));

	g_signal_connect(self, "activate", (GCallback) activate_cb, NULL);
}

/**
 * eina_application_new:
 * @application_id: ID for the application see gtk_application_new()
 *
 * Creates a new #EinaApplication with @application_id as ID
 *
 * Returns: A new #EinaApplication
 */
EinaApplication*
eina_application_new (const gchar *application_id)
{
	return g_object_new (EINA_TYPE_APPLICATION,
		"application-id", application_id,
		"flags", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_HANDLES_OPEN,
		NULL);
}

/**
 * eina_application_launch_for_uri:
 * @application: An #EinaApplication
 * @uri: URI to open
 * @error: Location for error
 *
 * Launches URI
 *
 * Returns: Successfull or not
 */
gboolean
eina_application_launch_for_uri(EinaApplication *application, const gchar *uri, GError **error)
{
 	return g_app_info_launch_default_for_uri(uri, NULL, error);
}


/**
 * eina_application_get_argc:
 * @self: An #EinaApplication
 *
 * Gets argc for current @self. The returned pointer is owned by
 * @application
 *
 * Returns: (transfer none): argc
 */
gint*
eina_application_get_argc(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return NULL;
}

/**
 * eina_application_get_argv:
 * @self: An #EinaApplication
 *
 * Gets argv for current @self. The returned pointer is owned by
 * @application
 *
 * Returns: (transfer none): argv
 */
gchar***
eina_application_get_argv(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	return NULL;
}

/**
 * eina_application_set_interface:
 * @self: An #EinaApplication
 * @name: The inteface's name. Must be unique in @self
 * @interface: (transfer none) (allow-none): Pointer to the interface
 *
 * Inserts an interface in the #EinaApplication object.
 * If @interface is %NULL it is deleted from @self and all references are
 * cleared
 *
 * Returns: %TRUE if successfull, %FALSE otherwise
 */
gboolean
eina_application_set_interface(EinaApplication *self, const gchar *name, gpointer interface)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), FALSE);
	g_return_val_if_fail(name != NULL, FALSE);

	if (interface)
	{
		g_return_val_if_fail(g_hash_table_lookup(self->priv->interfaces, name) == NULL, FALSE);
		g_hash_table_insert(self->priv->interfaces, g_strdup(name), interface);
	}
	else
		g_hash_table_remove(self->priv->interfaces, name);

	return TRUE;
}

/**
 * eina_application_get_interface: (skip):
 * @self: An #EinaApplication
 * @name: The interface's name
 *
 * Returns the references of an interface from the #EinaApplication object
 *
 * Returns: (transfer none): The pointer to the interface
 */
gpointer
eina_application_get_interface(EinaApplication *self, const gchar *name)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	gpointer interface = g_hash_table_lookup(self->priv->interfaces, name);
	g_return_val_if_fail(interface != NULL, NULL);

	return interface;
}

/**
 * eina_application_steal_interface:
 * @self: An #EinaApplication
 * @name: The interface's name
 *
 * Gets and delete the interface from the #EinaApplication. Its a combination
 * of eina_application_get_interface(self, name)
 * and eina_application_set_interface(self, name, NULL)
 *
 * Returns: (transfer full): The pointer to the interface
 */
gpointer
eina_application_steal_interface(EinaApplication *self, const gchar *name)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	// FIXME: Race condition
	gpointer ret = eina_application_get_interface(self, name);
	g_return_val_if_fail(ret != NULL, NULL);

	eina_application_set_interface(self, name, NULL);

	return ret;
}

/**
 * eina_application_get_settings:
 * @self: the #EinaApplication
 * @domain: string representing a domain
 *
 * Gets a GSettings for the domain, if it's not present it gets created.
 *
 * Returns: (transfer none): The #GSettings object
 */
GSettings*
eina_application_get_settings(EinaApplication *self, const gchar *domain)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	g_return_val_if_fail(domain != NULL, NULL);

	const gchar *app_id = g_application_get_application_id(G_APPLICATION(self));
	g_return_val_if_fail(app_id != NULL, NULL);

	GSettings *ret = g_hash_table_lookup(self->priv->settings, domain);
	if (ret == NULL)
	{
		ret = g_settings_new(domain);
		g_hash_table_insert(self->priv->settings, g_strdup(domain), ret);
	}

	return G_SETTINGS(ret);
}

/**
 * eina_application_get_window:
 * @self: the #EinaApplication
 *
 * Returns: (transfer none): Defaut window for #EinaApplication
 */
EinaWindow*
eina_application_get_window(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	create_window(self);

	return EINA_WINDOW(self->priv->window);
}

/**
 * eina_application_get_window_ui_manager:
 * @self: the #EinaApplication
 *
 * Returns: (transfer none): #GtkUIManager for default window of #EinaApplication
 */
GtkUIManager*
eina_application_get_window_ui_manager(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);

	return GTK_UI_MANAGER(eina_window_get_ui_manager(self->priv->window));
}

/**
 * eina_application_get_window_action_group:
 * @self: the #EinaApplication
 *
 * Returns: (transfer none): #GtkActionGroup for default window of #EinaApplication
 */
GtkActionGroup *
eina_application_get_window_action_group(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);

	return GTK_ACTION_GROUP(eina_window_get_action_group(self->priv->window));
}

static void
activate_cb(EinaApplication *self)
{
	g_return_if_fail(EINA_IS_APPLICATION(self));
	g_return_if_fail(EINA_IS_WINDOW(self->priv->window));

	gtk_window_present((GtkWindow *) self->priv->window);
}

static void
create_window(EinaApplication *self)
{
	g_return_if_fail(EINA_IS_APPLICATION(self));
	if (!GTK_IS_WINDOW(self->priv->window))
	{
		self->priv->window = (EinaWindow *) eina_window_new();

		gtk_application_add_window((GtkApplication *) self, (GtkWindow *) self->priv->window);
		g_signal_connect(self->priv->window, "delete-event", (GCallback) window_delete_event_cb, self);
	}
}

static gboolean
window_delete_event_cb(EinaWindow *window, GdkEvent *ev, EinaApplication *self)
{
	if (!eina_window_get_persistant(window))
	{
		g_application_release(G_APPLICATION(self));
		return TRUE;
	}

	return FALSE;
}

