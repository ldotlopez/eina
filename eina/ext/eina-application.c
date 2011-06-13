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

#include "eina-application.h"
#include <gel/gel.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <eina/ext/eina-window.h>

G_DEFINE_TYPE (EinaApplication, eina_application, GTK_TYPE_APPLICATION)

struct _EinaApplicationPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *interfaces;
	GHashTable *settings;

	EinaWindow *window;
};

static EinaWindow *
create_window(EinaApplication *self);
static gboolean
window_delete_event_cb(EinaWindow *window, GdkEvent *ev, EinaApplication *self);

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

static gboolean
eina_application_local_command_line(GApplication   *application, gchar ***arguments, gint *exit_status)
{
	/*
	If local_command_line() returns TRUE, the command line is expected to be
	completely handled, including possibly registering as the primary instance,
	calling g_application_activate() or g_application_open(), etc.

	If local_command_line() returns FALSE then the application is registered and
	the "command-line" signal is emitted in the primary instance (which may or
	may not be this instance). The signal handler gets passed a
	GApplicationCommandline object that (among other things) contains the
	remaining commandline arguments that have not been handled by local_command_line().
	*/

	gboolean new_instance = FALSE;
	gchar **argv = *arguments;

	for (guint i = 0; argv[i]; i++)
	{
		g_printf("  * '%s'\n", argv[i]);
		if (g_str_equal(argv[i], "-n") ||
		    g_str_equal(argv[i], "--new-instance"))
		{
			*arguments = gel_strv_delete(argv, i);
			new_instance = TRUE;
			break;
		}
	}

	if (new_instance)
	{
		const gchar *app_id = g_application_get_application_id(application);
		gchar *new_id = g_strdup_printf("%s.id%d", app_id, getpid());
		g_application_set_application_id(application, new_id);
		g_free(new_id);
	}

	return FALSE;
}

static void
eina_application_class_init (EinaApplicationClass *klass)
{
	GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
	GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaApplicationPrivate));

	object_class->dispose = eina_application_dispose;
 	application_class->local_command_line = eina_application_local_command_line;
}

static void
eina_application_init (EinaApplication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_APPLICATION, EinaApplicationPrivate);
	self->priv->interfaces = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	self->priv->settings   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

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
 * Returns %TRUE if successfull, %FALSE otherwise
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
 * eina_application_get_interface:
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
 * Returns: (transfer none): The pointer to the interface
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
 *
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
 *
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
 *
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

static EinaWindow *
create_window(EinaApplication *self)
{
	if (self->priv->window)
		return self->priv->window;

	self->priv->window = (EinaWindow *) eina_window_new();
	
	gtk_application_add_window((GtkApplication *) self, (GtkWindow *) self->priv->window);
	g_signal_connect(self->priv->window, "delete-event", (GCallback) window_delete_event_cb, self);

	return EINA_WINDOW(self->priv->window);
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


