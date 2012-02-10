/*
 * eina/ext/eina-file-chooser-dialog.c
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
 * SECTION:eina-file-chooser-dialog
 * @title: EinaFileChooserDialog
 * @short_description: Specialized FileChooser
 *
 * #EinaFileChooserDialog is an specialized #GtkFileChooserDialog providing
 * async, recursive and filtering URI selection.
 *
 * Other UI improvements include the posibility of showing messages or
 * canceling current operation
 *
 * See also: #GtkFileChooserDialog
 */

#include "eina-file-chooser-dialog.h"
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>
#include <gel/gel-fs-scanner.h>
#include "eina-stock.h"
#include "eina-file-utils.h"

G_DEFINE_TYPE (EinaFileChooserDialog, eina_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)

struct _EinaFileChooserDialogPrivate {
	EinaFileChooserDialogAction action;
	GList  *uris;     // Store output from eina_file_chooser_dialog_run
	GtkBox *info_box; // Info widgets

	gboolean in_nested_loop;
	GCancellable *cancellable;
};

/*
 * Private API
 */
void
_set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action);
static void
_set_sensitive(EinaFileChooserDialog *self, gboolean value);
static void
_message_clear(EinaFileChooserDialog *self);
static void
_block_ui(EinaFileChooserDialog *self);
static void
_unblock_ui(EinaFileChooserDialog *self);

/*
 * GelFSScanner stuff
 */
static void
scanner_ready_cb(GList *output, EinaFileChooserDialog *self);
static gboolean
scanner_filter_cb(GFile *f);

static void
free_resources(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = self->priv;

	if (priv->uris)
	{
		g_list_foreach(priv->uris, (GFunc) g_free, NULL);
		g_list_free(priv->uris);
		priv->uris = NULL;
	}
}

static void
eina_file_chooser_dialog_dispose (GObject *object)
{
	free_resources(EINA_FILE_CHOOSER_DIALOG(object));

	if (G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->dispose (object);
}

static void
eina_file_chooser_dialog_finalize (GObject *object)
{
	free_resources(EINA_FILE_CHOOSER_DIALOG(object));

	if (G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->finalize)
		G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->finalize (object);
}

static void
eina_file_chooser_dialog_class_init (EinaFileChooserDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (EinaFileChooserDialogPrivate));

	object_class->dispose = eina_file_chooser_dialog_dispose;
	object_class->finalize = eina_file_chooser_dialog_finalize;

	eina_stock_init();
}

static void
eina_file_chooser_dialog_init (EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogPrivate);

	#if defined(__APPLE__) || defined(__APPLE_CC_)
	g_signal_connect(GTK_FILE_CHOOSER(self), "selection-changed", (GCallback) gtk_widget_queue_draw, NULL);
	#endif

	priv->info_box = (GtkBox   *) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous(priv->info_box, FALSE);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));

	gtk_box_pack_end(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))), GTK_WIDGET(priv->info_box), FALSE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))), GTK_WIDGET(priv->info_box), 1);
	gtk_window_set_icon_name(GTK_WINDOW(self), GTK_STOCK_OPEN);
}

/*
 * eina_file_chooser_dialog_new:
 * @parent: Parent #GtkWindow or %NULL
 * @action: Action to perform
 *
 * Creates new #EinaFileChooserDialog designed to perform a concrete action
 *
 * Returns: The new EinaFileChooserDialog
 */
EinaFileChooserDialog*
eina_file_chooser_dialog_new (GtkWindow *parent, EinaFileChooserDialogAction action)
{
	EinaFileChooserDialog *self = g_object_new (EINA_TYPE_FILE_CHOOSER_DIALOG, "local-only", FALSE, NULL);
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogPrivate);
	self->priv->action = action;

	g_object_set((GObject *) self,
		"transient-for", parent,
		"modal", FALSE,
		"destroy-with-parent", TRUE,
		NULL);
	_set_action(self, action);

	return self;
}

/*
 * eina_file_chooser_dialog_set_custom_msg:
 * @self: An #EinaFileChooserDialog
 * @image: Widget (usually an image) to display at the left of the message
 *          area
 * @label: Label with the message to display.
 * @action: Widget (usually a button) to perform an action (close message,
 *          etc...)
 *
 * Display a custom message in the message area of the #EinaFileChooserDialog
 */
void eina_file_chooser_dialog_set_custom_msg(EinaFileChooserDialog *self,
	GtkWidget *image, GtkLabel *label, GtkWidget *action)
{
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self));
	g_return_if_fail(GTK_IS_WIDGET(image));
	g_return_if_fail(GTK_IS_LABEL(label));
	g_return_if_fail(GTK_IS_WIDGET(action));

	EinaFileChooserDialogPrivate *priv = self->priv;

	_message_clear(self);

	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(image),  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(label),  TRUE,  TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(action), FALSE, FALSE, 0);

	gtk_widget_show_all(GTK_WIDGET(priv->info_box));
}

/*
 * eina_file_chooser_dialog_set_msg:
 * @self: An #EinaFileChooserDialog
 * @type: Type of the message
 * @msg: Message to display
 *
 * Displays a standard message of type @type in the message area of the
 * filechooser
 */
void eina_file_chooser_dialog_set_msg(EinaFileChooserDialog *self,
	EinaFileChooserDialogMsgType type, const gchar *msg)
{
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self));
	g_return_if_fail(msg != NULL);

	EinaFileChooserDialogPrivate *priv = self->priv;

	gchar *stock_id;
	switch (type)
	{
	case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO:
		stock_id = GTK_STOCK_INFO;
		break;

	case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_WARN:
		stock_id = GTK_STOCK_DIALOG_WARNING;
		break;

	case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_ERROR:
		stock_id = GTK_STOCK_DIALOG_ERROR;
		break;

	case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_NONE:
	default:
		gtk_widget_hide(GTK_WIDGET(priv->info_box));
		return;
	}

	GtkButton *button = (GtkButton *) gtk_button_new();
	gtk_button_set_relief(button, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock("gtk-close", GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(button, "clicked", (GCallback) _message_clear, self);

	eina_file_chooser_dialog_set_custom_msg(self,
		gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU),
		(GtkLabel *) gtk_label_new(msg),
		GTK_WIDGET(button));
}

/*
 * eina_file_chooser_dialog_get_uris:
 * @self: An #EinaFileChooserDialog
 *
 * Gets the selected URIs from the filechooser dialog recursivelly.
 * This function is synchronous, while transversing the tree it puts itself in
 * a unsensitive state and enters in deeper gtk main-loop.
 *
 * Operation is cancellable using a button.
 *
 * Returns: (transfer full) (element-type utf-8): The URIs. Elements of the
 * list must be freed with g_free and list with g_list_free
 */
GList *
eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self)
{
	g_return_val_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self), NULL);

	EinaFileChooserDialogPrivate *priv = self->priv;
	free_resources(self);

        GSList *uris = g_slist_sort(gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(self)), (GCompareFunc) strcmp);
	GList  *file_objects = NULL;
	for (GSList *iter = uris; iter != NULL; iter = iter->next)
		file_objects = g_list_prepend(file_objects, g_file_new_for_uri((gchar *) iter->data));

	g_slist_foreach(uris, (GFunc) g_free, NULL);
	g_slist_free(uris);

	file_objects = g_list_reverse(file_objects);

	g_warn_if_fail(priv->cancellable == NULL);
	priv->cancellable = g_cancellable_new();
	gel_fs_scanner_scan(file_objects, priv->cancellable,
		(GelFSScannerReadyFunc) scanner_ready_cb,
		(GCompareFunc) gel_fs_scaner_compare_gfile_by_name, /* compare_func */
		(GSourceFunc) scanner_filter_cb, /* filter_func */
		self, NULL);

	g_list_foreach(file_objects, (GFunc) g_object_unref, NULL);
	g_list_free(file_objects);

	// The 'cancel' button
	GtkButton *cancel = (GtkButton *) gtk_button_new();
	gtk_button_set_relief(cancel, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(cancel), gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(cancel, "clicked", (GCallback) g_cancellable_cancel, priv->cancellable);

	// Show busy UI
	eina_file_chooser_dialog_set_custom_msg(self,
		g_object_new(GTK_TYPE_SPINNER, "active", TRUE, NULL),
		(GtkLabel *) gtk_label_new(_("Loading files...")),
		GTK_WIDGET(cancel));

	_block_ui(self);

	gboolean user_cancel = g_cancellable_is_cancelled(priv->cancellable);
	gel_free_and_invalidate(priv->cancellable, NULL, g_object_unref);
	if (user_cancel)
	{
		_message_clear(self);
		free_resources(self);
		return NULL;
	}

	/*
	 * Create custom message for UI feedback
	 */
	GtkButton *close = (GtkButton *) gtk_button_new();
	gtk_button_set_relief(close, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(close), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(close, "clicked", (GCallback) _message_clear, self);

	gchar *str = g_strdup_printf(N_("Loaded %d streams"), g_list_length(priv->uris));
	eina_file_chooser_dialog_set_custom_msg(self,
		gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU),
		(GtkLabel *) gtk_label_new(str),
		GTK_WIDGET(close));
	g_free(str);

	/*
	 * Return URLs
	 */
	GList *ret = priv->uris;
	priv->uris = NULL;

	return ret;
}

// --
// Internal
// --

/**
 * _set_action:
 * @self: An #EinaFileChooserDialog
 * @action: Action type
 *
 * Sets the action for the dialog. Currently only
 * %EINA_FILE_CHOOSER_DIALOG_ACTION_LOAD_FILES is supported
 */
void
_set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action)
{
	GtkFileFilter *filter;
	const gchar *music_folder;

	if ((music_folder = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)) == NULL)
		music_folder = g_get_home_dir();

	switch (action)
	{
	// Load or queue
	case EINA_FILE_CHOOSER_DIALOG_ACTION_LOAD_FILES:
		filter = gtk_file_filter_new();
		gtk_file_filter_add_mime_type(filter, "audio/*");
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(self), filter);
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(self), music_folder, NULL);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(self), music_folder);
		gtk_file_chooser_set_action(GTK_FILE_CHOOSER(self), GTK_FILE_CHOOSER_ACTION_OPEN);
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(self), TRUE);

		gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_MEDIA_PLAY, EINA_FILE_CHOOSER_RESPONSE_PLAY);
		gtk_dialog_add_button(GTK_DIALOG(self), EINA_STOCK_QUEUE, EINA_FILE_CHOOSER_RESPONSE_QUEUE);
		gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

		gtk_window_set_title(GTK_WINDOW(self), _("Add or queue files"));
		break;

	default:
		g_warning("EinaFileChooserDialog unknow action %d", action);
	}
}

/**
 * _set_sensitive:
 * @self: An #EinaFileChooserDialog
 * @value: Value for sensitiviness
 *
 * Sets the sensitiviness for @self
 */
static void
_set_sensitive(EinaFileChooserDialog *self, gboolean value)
{
	EinaFileChooserDialogPrivate *priv = self->priv;

	gtk_widget_set_sensitive(gtk_dialog_get_action_area(GTK_DIALOG(self)), value);
	GList *children = gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))));
	GList *l = children;
	while (l)
	{
		if (l->data != priv->info_box)
			gtk_widget_set_sensitive(GTK_WIDGET(l->data), value);
		l = l->next;
	}
	g_list_free(children);

	if (value == TRUE)
	{
		gdk_window_set_cursor(
			gtk_widget_get_window(GTK_WIDGET(self)),
			NULL);
		g_signal_handlers_disconnect_by_func(self, gtk_true, self);
	}
	else
	{
		GtkWidget *w = GTK_WIDGET(self);
		GdkWindow *win = gtk_widget_get_window(GTK_WIDGET(self));
		GdkDisplay *display = gtk_widget_get_display(w);
		gdk_window_set_cursor(win, gdk_cursor_new_for_display(display, GDK_WATCH));
		g_signal_connect(self, "delete-event", G_CALLBACK(gtk_true), self);
	}
}

/**
 * _message_clear:
 * @self: An #EinaFileChooserDialog
 *
 * Clears any previous message from @self
 */
static void
_message_clear(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = self->priv;
	GList *children = gtk_container_get_children(GTK_CONTAINER(priv->info_box));
	GList *iter = children;
	while (iter)
	{
		gtk_container_remove(GTK_CONTAINER(priv->info_box), GTK_WIDGET(iter->data));
		iter = iter->next;
	}
	g_list_free(children);
}

/**
 * _block_ui:
 * @self: An #EinaFileChooserDialog
 *
 * Sets UI sensitiviness to FALSE and enters in a nested main loop
 */
static void
_block_ui(EinaFileChooserDialog *self)
{
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self));
	EinaFileChooserDialogPrivate *priv = self->priv;

	g_return_if_fail(priv->in_nested_loop == FALSE);
	priv->in_nested_loop = TRUE;

	_set_sensitive(self, FALSE);
	gtk_main();
}

/**
 * _unblock_ui:
 * @self: An #EinaFileChooserDialog
 *
 * Sets UI sensitiviness to TRUE and exists from a previous nested main loop
 */
static void
_unblock_ui(EinaFileChooserDialog *self)
{
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self));
	EinaFileChooserDialogPrivate *priv = self->priv;

	g_return_if_fail(priv->in_nested_loop == TRUE);
	priv->in_nested_loop = FALSE;

	_set_sensitive(self, TRUE);
	gtk_main_quit();
}

/*
 * GelFSScanner stuff
 */
static void
scanner_ready_cb(GList *output, EinaFileChooserDialog *self)
{
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(self));
	EinaFileChooserDialogPrivate *priv = self->priv;

	g_warn_if_fail(priv->uris == NULL);

	/* Extract uris and save them */
	for (GList *iter = output; iter != NULL; iter = iter->next)
	{
		GFile *f = G_FILE(iter->data);
		if (!f)
			continue;

		priv->uris = g_list_prepend(priv->uris, g_file_get_uri(f));
	}
	priv->uris = g_list_reverse(priv->uris);

	_unblock_ui(self);
}

static gboolean
scanner_filter_cb(GFile *f)
{
	g_return_val_if_fail(G_IS_FILE(f), FALSE);

	GFileInfo *info = g_object_get_data((GObject *) f, GEL_FS_SCANNER_INFO_KEY);
	g_return_val_if_fail(G_IS_FILE_INFO(info), FALSE);

	if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
		return TRUE;

	if (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR)
		return eina_file_utils_is_supported_file(f);

	else
		return FALSE;
}

