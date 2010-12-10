#include "eina-application.h"
#include <gel/gel.h>
#include <glib/gi18n.h>
#include <eina/ext/eina-window.h>

G_DEFINE_TYPE (EinaApplication, eina_application, GTK_TYPE_APPLICATION)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_APPLICATION, EinaApplicationPrivate))

struct _EinaApplicationPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *interfaces;
	GHashTable *settings;

	EinaWindow *window;
};

static EinaWindow *
create_window(EinaApplication *self);

static void
eina_application_dispose (GObject *object)
{
	EinaApplication *self = EINA_APPLICATION(object);

	gel_free_and_invalidate(self->priv->interfaces, NULL, g_hash_table_destroy);
	gel_free_and_invalidate(self->priv->settings,   NULL, g_hash_table_destroy);

	G_OBJECT_CLASS (eina_application_parent_class)->dispose (object);
}

static void
eina_application_class_init (EinaApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaApplicationPrivate));

	object_class->dispose = eina_application_dispose;
}

static void
eina_application_init (EinaApplication *self)
{
	self->priv = GET_PRIVATE(self);
	self->priv->interfaces = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	self->priv->settings   = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

	// Generate iconlist for windows
	const gint sizes[] = { 16, 32, 48, 64, 128 };
	GList *icon_list = NULL;
	gchar *icon_filename = gel_resource_locate(GEL_RESOURCE_IMAGE, "eina.svg");
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

EinaApplication*
eina_application_new (const gchar *application_id)
{
	return g_object_new (EINA_TYPE_APPLICATION,
		"application-id", application_id,
		"flags", G_APPLICATION_FLAGS_NONE,
		NULL);
}

gint*
eina_application_get_argc(EinaApplication *application)
{
	return NULL;
}

gchar***
eina_application_get_argv(EinaApplication *application)
{
	return NULL;
}

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

gpointer
eina_application_get_interface(EinaApplication *self, const gchar *name)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	gpointer interface = g_hash_table_lookup(self->priv->interfaces, name);
	g_return_val_if_fail(interface != NULL, NULL);
	return interface;
}

gpointer
eina_application_steal_interface(EinaApplication *self, const gchar *name)
{
	// XXX: Race condition?
	gpointer ret = eina_application_get_interface(self, name);
	g_return_val_if_fail(ret != NULL, NULL);

	eina_application_set_interface(self, name, NULL);
	return ret;
}

/*
 * eina_application_get_settings:
 *
 * @self: (inout) (transfer none): the #EinaApplication
 * @domain: (in) (transfer none): string representing a domain
 *
 * Gets a GSettings for the domain, if it's not present it gets created.
 *
 * Returns: (transfer none): The #GSettings object
 */
GSettings*
eina_application_get_settings(EinaApplication *self, const gchar *domain)
{
	gchar *app_id = NULL;
	g_object_get(self, "application-id", &app_id, NULL);
	g_return_val_if_fail(app_id != NULL, NULL);

	GSettings *ret = g_hash_table_lookup(self->priv->settings, domain);
	if (ret == NULL)
	{
		ret = g_settings_new(domain);
		g_hash_table_insert(self->priv->settings, g_strdup(domain), ret);
	}

	return ret;
}

static EinaWindow *
create_window(EinaApplication *self)
{
	if (self->priv->window)
		return self->priv->window;

	self->priv->window = (EinaWindow *) eina_window_new();
	gtk_application_add_window((GtkApplication *) self, (GtkWindow *) self->priv->window);
	return self->priv->window;
}

/*
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
	return self->priv->window;
}

/*
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
	return eina_window_get_ui_manager(self->priv->window);
}
