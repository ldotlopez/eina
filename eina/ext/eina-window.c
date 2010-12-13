#include "eina-window.h"
#include <glib/gi18n.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-stock.h>

G_DEFINE_TYPE (EinaWindow, eina_window, GTK_TYPE_WINDOW)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_WINDOW, EinaWindowPrivate))

struct _EinaWindowPrivate {
	GtkBox    *container;

	GtkActionGroup *ag;
	GtkUIManager   *ui_manager;
};

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
eina_window_dispose (GObject *object)
{
	EinaWindow *self = EINA_WINDOW(object);

	gel_free_and_invalidate(self->priv->ag,         NULL, g_object_unref);
	gel_free_and_invalidate(self->priv->ui_manager, NULL, g_object_unref);
	G_OBJECT_CLASS (eina_window_parent_class)->dispose (object);
}

static void
eina_window_class_init (EinaWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaWindowPrivate));

	/*
	 * EinaWindow::activate:
	 *
	 * Emitted if some action is activated
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

	object_class->dispose = eina_window_dispose;
}

static void
eina_window_init (EinaWindow *self)
{
	self->priv = GET_PRIVATE(self);

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
		(GtkWidget *) (self->priv->container = (GtkBox *) gtk_vbox_new(FALSE, 0)),
		TRUE, TRUE, 0
		);

	gtk_widget_show_all((GtkWidget *) tmpbox);
	gtk_window_add_accel_group((GtkWindow *) self, gtk_ui_manager_get_accel_group(self->priv->ui_manager));
}

EinaWindow*
eina_window_new (void)
{
	return g_object_new (EINA_TYPE_WINDOW, NULL);
}

/*
 * eina_window_get_window_ui_manager:
 *
 * @self: the #EinaWindow
 *
 * Returns: (transfer none): #GtkUIManager for default window of #EinaWindow
 */
GtkUIManager*
eina_window_get_ui_manager(EinaWindow *self)
{
	g_return_val_if_fail(EINA_IS_WINDOW(self), NULL);
	return GTK_UI_MANAGER(self->priv->ui_manager);
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
		GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Select plugins"),
			(GtkWindow *) self,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);
		GtkWidget *pm = (GtkWidget *) gel_ui_plugin_manager_new(gel_plugin_engine_get_default());
		gtk_box_pack_start((GtkBox*) gtk_dialog_get_content_area((GtkDialog *) dialog),
			pm,
			TRUE, TRUE, 0);

		GdkScreen *screen = gtk_window_get_screen((GtkWindow *) dialog);
		gint w = gdk_screen_get_width(screen)  / 4;
		gint h = gdk_screen_get_height(screen) / 2;
		gtk_window_resize((GtkWindow *) dialog, w, h);

		gtk_widget_show(pm);
		gtk_dialog_run((GtkDialog *) dialog);
		gtk_widget_destroy(dialog);

		return;
	}

	g_warning(N_("Unhandled unknow action '%s'"), name);
}

