#include "eina-application.h"
#include <gel/gel.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaApplication, eina_application, GTK_TYPE_APPLICATION)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_APPLICATION, EinaApplicationPrivate))

struct _EinaApplicationPrivate {
	gint    *argc;
	gchar ***argv;

	GHashTable *interfaces;
	GHashTable *settings;

	GtkWindow *default_window;
	GtkBox    *container;

	GtkActionGroup *ag;
	GtkUIManager   *ui_manager;
};

static GtkWindow *
create_window(EinaApplication *self);
static void
action_activated_cb(GtkAction *action, EinaApplication *self);

static gchar *ui_mng_str =
"<ui>"
"  <menubar name='Main' >"
"    <menu name='File' action='file-menu' >"
"      <menuitem name='Quit' action='quit-action' />"
"    </menu>"
"    <menu name='Edit' action='edit-menu' >"
"    </menu>"
"    <menu name='Plugins' action='plugins-menu' >"
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
	{ "quit-action",  GTK_STOCK_QUIT, NULL,           NULL,     NULL, (GCallback) action_activated_cb }
};

enum {
	ACTION_ACTIVATE,
	LAST_SIGNAL
};
guint application_signals[LAST_SIGNAL] = { 0 };

static void
eina_application_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_application_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_application_dispose (GObject *object)
{
	EinaApplication *self = EINA_APPLICATION(object);

	gel_free_and_invalidate(self->priv->interfaces, NULL, g_hash_table_destroy);
	gel_free_and_invalidate(self->priv->ag,         NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->ui_manager, NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->settings,   NULL, g_hash_table_destroy);


	G_OBJECT_CLASS (eina_application_parent_class)->dispose (object);
}

static void
eina_application_class_init (EinaApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaApplicationPrivate));

	object_class->get_property = eina_application_get_property;
	object_class->set_property = eina_application_set_property;
	object_class->dispose = eina_application_dispose;

    application_signals[ACTION_ACTIVATE] =
		g_signal_new ("action-activate",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaApplicationClass, action_activate),
		NULL, NULL,
		gel_marshal_BOOLEAN__OBJECT,
		G_TYPE_BOOLEAN,
		1,
		G_TYPE_OBJECT);
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

static GtkWindow *
create_window(EinaApplication *self)
{
	if (self->priv->default_window)
		return self->priv->default_window;

	self->priv->default_window = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);

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

	GtkBox *tmpbox = (GtkBox *) gtk_vbox_new(FALSE, 0);

	gtk_container_add(
		(GtkContainer *) self->priv->default_window,
		(GtkWidget *) tmpbox
		);

	gtk_box_pack_start(
		tmpbox,
		gtk_ui_manager_get_widget(self->priv->ui_manager, "/Main"),
        FALSE, TRUE, 0
		);
	gtk_box_pack_start(
		tmpbox,
		(GtkWidget *) (self->priv->container = (GtkBox *) gtk_vbox_new(FALSE, 0)),
		TRUE, TRUE, 0
		);

	gtk_widget_show_all((GtkWidget *) tmpbox);
	gtk_window_add_accel_group(self->priv->default_window, gtk_ui_manager_get_accel_group(self->priv->ui_manager));

	return self->priv->default_window;
}


/*
 * eina_application_get_window:
 *
 * @self: the #EinaApplication
 *
 * Returns: (transfer none): Defaut window for #EinaApplication
 */
GtkWindow*
eina_application_get_window(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	create_window(self);
	return self->priv->default_window;
}

/*
 * eina_application_get_window_ui_manager:
 *
 * @self: the #EinaApplication
 *
 * Returns: (transfer none): #GtkUIManager for default window of #EinaApplication
 */
GtkUIManager*
eina_application_get_window_ui_manager  (EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	create_window(self);
	return GTK_UI_MANAGER(self->priv->ui_manager);
}

/*
 * eina_application_get_window_content_area:
 *
 * @self: (transfer none) (inout): the #EinaApplication
 *
 * Returns: (transfer none): #GtkVBox representing content area for default window of #EinaApplication
 */
GtkVBox*
eina_application_get_window_content_area(EinaApplication *self)
{
	g_return_val_if_fail(EINA_IS_APPLICATION(self), NULL);
	create_window(self);
	return GTK_VBOX(self->priv->container);
}

static void
action_activated_cb(GtkAction *action, EinaApplication *self)
{
	g_return_if_fail(EINA_IS_APPLICATION(self));
	g_return_if_fail(GTK_IS_ACTION(action));

	gboolean ret = FALSE;
	g_signal_emit(self, application_signals[ACTION_ACTIVATE], 0, action, &ret);

	if (ret)
		return;

	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "quit-action"))
	{
		g_application_release(G_APPLICATION(self));
		return;
	}

	g_warning(N_("Unhandled unknow action '%s'"), name);
}


