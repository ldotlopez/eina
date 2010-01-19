/*
 * eina/ext/eina-file-chooser-dialog.c
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define GEL_DOMAIN "Eina::FileChooserDialog"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-stock.h>
#include <eina/ext/eina-file-chooser-dialog.h>

G_DEFINE_TYPE (EinaFileChooserDialog, eina_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogPrivate))

typedef struct _EinaFileChooserDialogPrivate EinaFileChooserDialogPrivate;

struct _EinaFileChooserDialogPrivate {
	EinaFileChooserDialogAction action;
	GSList *uris; // Store output from eina_file_chooser_dialog_run 

	gboolean      user_cancel;
	GtkBox       *info_box;    // Info widgets
	GQueue       *queue;       // Queue previous to query operation
	GelIOTreeOp  *op;          // RecurseTree operation in progress
};

void
set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action);

static void
update_sensitiviness(EinaFileChooserDialog *self, gboolean value);
static void
clear_message(EinaFileChooserDialog *self);
static void
run_queue(EinaFileChooserDialog *self);

static void
cancel_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self);

// Async ops
static void
tree_op_success_cb(GelIOTreeOp *op, const GFile *source, const GNode  *result, gpointer data);
static void
tree_op_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error,  gpointer data);

// Comparables
static gint
g_node_cmp_rev(GNode *a, GNode *b);

static void
reset_resources(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	if (priv->op)
	{
		gel_io_tree_op_close(priv->op);
		priv->op = NULL;
	}

	if (priv->queue && !g_queue_is_empty(priv->queue))
	{
		g_queue_foreach(priv->queue, (GFunc) g_free, NULL);
		g_queue_clear(priv->queue);
	}

	if (priv->uris)
	{
		g_slist_foreach(priv->uris, (GFunc) g_free, NULL);
		g_slist_free(priv->uris);
		priv->uris = NULL;
	}
}

static void
free_resources(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	reset_resources(self);

	if (priv->queue)
	{
		g_queue_free(priv->queue);
		priv->queue = NULL;
	}
}

#if !GTK_CHECK_VERSION(2,14,0)
static GObject*
eina_file_chooser_dialog_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	gint i;
	GObject *obj;

	/* Override file-system-backend */
	for (i = 0 ; i < n_properties; i++)
	{
		if (g_str_equal(properties[i].pspec->name, "file-system-backend"))
			g_value_set_static_string(properties[i].value, "gio");
	}
	
    {
		/* Chain up to the parent constructor */
		EinaFileChooserDialogClass *klass;
		GObjectClass *parent_class;  
		klass = EINA_FILE_CHOOSER_DIALOG_CLASS(g_type_class_peek(EINA_TYPE_FILE_CHOOSER_DIALOG));
		parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
		obj = parent_class->constructor (gtype, n_properties, properties);
	}

	return obj;
}
#endif

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

	#if !GTK_CHECK_VERSION(2,14,0)
	object_class->constructor = eina_file_chooser_dialog_constructor;
	#endif
	object_class->dispose = eina_file_chooser_dialog_dispose;
	object_class->finalize = eina_file_chooser_dialog_finalize;

	eina_stock_init();
}

static void
eina_file_chooser_dialog_init (EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	#if defined(__APPLE__) || defined(__APPLE_CC_)
	g_signal_connect(GTK_FILE_CHOOSER(self), "selection-changed", (GCallback) gtk_widget_queue_draw, NULL);
	#endif

	priv->queue = g_queue_new();
	priv->info_box = (GtkBox   *) gtk_hbox_new(FALSE, 5);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));

	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), FALSE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), 1);
	gtk_window_set_icon_name(GTK_WINDOW(self), GTK_STOCK_OPEN);
}

EinaFileChooserDialog*
eina_file_chooser_dialog_new (EinaFileChooserDialogAction action)
{
	EinaFileChooserDialog *self;
	EinaFileChooserDialogPrivate *priv;

	self = g_object_new (EINA_TYPE_FILE_CHOOSER_DIALOG, "local-only", FALSE, NULL);
	priv = GET_PRIVATE(self);
	priv->action = action;

	set_action(self, action);

	return self;
}

void eina_file_chooser_dialog_set_custom_msg(EinaFileChooserDialog *self,
	GtkImage *image, GtkLabel *label, GtkWidget *action)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	clear_message(self);

	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(image),  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(label),  TRUE,  TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(action), FALSE, FALSE, 0);

	gtk_widget_show_all(GTK_WIDGET(priv->info_box));
}

void eina_file_chooser_dialog_set_msg(EinaFileChooserDialog *self,
	EinaFileChooserDialogMsgType type, gchar *msg)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	GtkButton *button;
	gchar *stock_id;

	switch (type) {
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

	button = (GtkButton *) gtk_button_new();
	gtk_button_set_relief(button, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_stock("gtk-close", GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(button, "clicked", (GCallback) clear_message, self);

	eina_file_chooser_dialog_set_custom_msg(self,
		(GtkImage *) gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU),
		(GtkLabel *) gtk_label_new(msg),
		GTK_WIDGET(button)
		);
}

GSList *
eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	reset_resources(self);

	GSList *selected = g_slist_sort(gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(self)), (GCompareFunc) strcmp);

	// Build a queue with URIs
	GSList *iter  = selected;
	while (iter)
	{
		g_queue_push_tail(priv->queue, iter->data);
		iter = iter->next;
	}
	g_slist_free(selected);

	// Put UI in busy state
	update_sensitiviness(self, FALSE);

	// Loading image
	gchar *loading_path = gel_resource_locate(GEL_RESOURCE_IMAGE, "loading-spin-16x16.gif");
	GtkImage *loading = (GtkImage *) gtk_image_new_from_file(loading_path);
	g_free(loading_path);

	// The 'cancel' button
	GtkButton *cancel = (GtkButton *) gtk_button_new();
	gtk_button_set_relief(cancel, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(cancel), gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU));
	g_signal_connect(cancel, "clicked", (GCallback) cancel_button_clicked_cb, self);

	// Show busy UI
	eina_file_chooser_dialog_set_custom_msg(self,
		loading,
		(GtkLabel *) gtk_label_new(_("Loading files...")),
		GTK_WIDGET(cancel));

	run_queue(self);
	gtk_main();

	update_sensitiviness(self, TRUE);

	if (priv->user_cancel)
	{
		clear_message(self);
		priv->user_cancel = FALSE;
		reset_resources(self);
		return NULL;
	}

	GSList *ret = NULL;
	iter = priv->uris;
	while (iter)
	{
		ret = g_slist_prepend(ret, g_strdup((gchar *) iter->data));
		iter = iter->next;
	}
	return g_slist_reverse(ret);
}

// --
// Internal
// --
void
set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action)
{
	GtkFileFilter *filter;
	const gchar *music_folder;

	if ((music_folder = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)) == NULL)
		music_folder = g_get_home_dir();

	switch (action)
	{
	// Load or queue
	case EINA_FILE_CHOOSER_DIALOG_LOAD_FILES:
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

static void
update_sensitiviness(EinaFileChooserDialog *self, gboolean value)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	gtk_widget_set_sensitive(GTK_DIALOG(self)->action_area, value);
	GList *children = gtk_container_get_children(GTK_CONTAINER(GTK_DIALOG(self)->vbox));
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
			#if GTK_CHECK_VERSION(2,14,0)
			gtk_widget_get_window(GTK_WIDGET(self)),
			#else
			GTK_WIDGET(self)->window,
			#endif
			NULL);
		g_signal_handlers_disconnect_by_func(self, gtk_true, self);
	}
	else
	{
		GtkWidget *w = GTK_WIDGET(self);
		#if GTK_CHECK_VERSION(2,14,0)
		GdkWindow *win = gtk_widget_get_window(GTK_WIDGET(self));
		#else
		GdkWindow *win = GTK_WIDGET(self)->window;
		#endif
		GdkDisplay *display = gtk_widget_get_display(w);
		gdk_window_set_cursor(win, gdk_cursor_new_for_display(display, GDK_WATCH));
		g_signal_connect(self, "delete-event", G_CALLBACK(gtk_true), self);
	}
}

static void
clear_message(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	GList *children = gtk_container_get_children(GTK_CONTAINER(priv->info_box));
	GList *iter = children;
	while (iter)
	{
		gtk_container_remove(GTK_CONTAINER(priv->info_box), GTK_WIDGET(iter->data));
		iter = iter->next;
	}
	g_list_free(children);
}

static void
run_queue(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	// Exit nested main loop
	if (g_queue_is_empty(priv->queue) && (gtk_main_level() > 1))
	{
		gint len = g_slist_length(priv->uris);
		gchar *msg = g_strdup_printf(N_("Found %d streams"), len);
		eina_file_chooser_dialog_set_msg(self,  EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, msg);
		g_free(msg);
		
		gtk_main_quit();
		return;
	}

	// Process next element
	gchar *uri = g_queue_pop_head(priv->queue);
	if (priv->op)
	{
		g_warning("Operation in progress on run_queue call");
		gel_io_tree_op_close(priv->op);
	}
	
	priv->op = gel_io_tree_walk(g_file_new_for_uri(uri), "standard::*", TRUE,
		tree_op_success_cb, tree_op_error_cb, self);

	g_free(uri);
}

static void
cancel_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	priv->user_cancel = TRUE;
	reset_resources(self);
	clear_message(self);
	update_sensitiviness(self, TRUE);
}

// --
// Handle async reads
// --
static void
parse_tree(EinaFileChooserDialog *self, GNode *result)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	gchar *uri = g_file_get_uri(G_FILE(result->data));

	GFileInfo *info = gel_file_get_file_info(G_FILE(result->data));
	g_return_if_fail(info);

	GFileType  type = g_file_info_get_file_type(info);

	// Input is regular, nothing to recurse, just prepend to results
	if (type == G_FILE_TYPE_REGULAR)
	{
		priv->uris = g_slist_prepend(priv->uris, uri);
		g_free(uri);
		return;
	}

	g_free(uri); // Useless since we have a directory
	g_return_if_fail(type == G_FILE_TYPE_DIRECTORY);

	// Generate a list with sorted children
	GList *children = NULL;
	GNode *n = result->children;
	while (n)
	{
		children = g_list_prepend(children, n);
		n = n->next;
	}
	children = g_list_sort(children, (GCompareFunc) g_node_cmp_rev);

	// Foreach children check type
	GList *iter = children;
	while (iter)
	{
		GNode *node      = (GNode *) iter->data;
		GFile *file      = G_FILE(node->data);
		GFileInfo *info  = gel_file_get_file_info(file);
		GFileType type   = g_file_info_get_file_type(info);

		if (type == G_FILE_TYPE_REGULAR)
			priv->uris = g_slist_prepend(priv->uris, g_file_get_uri(file));

		if (type == G_FILE_TYPE_DIRECTORY)
			parse_tree(self, node);
		iter = iter->next;
	}
	g_list_free(children);
}

// Walk over GelIORecurseTree and add files
static void
tree_op_success_cb(GelIOTreeOp *op, const GFile *source, const GNode  *result, gpointer data)
{
	EinaFileChooserDialog *self = EINA_FILE_CHOOSER_DIALOG(data);
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	// Get and parse result
	parse_tree(EINA_FILE_CHOOSER_DIALOG(data), (GNode *) result);

	// Close this operation and go to next element in queue
	gel_io_tree_op_close(op);
	priv->op = NULL;

	run_queue(self);
}

// Show errors from GelIOOp and update UI to reflect them
static void
tree_op_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error,  gpointer data)
{
	gchar *uri = g_file_get_uri((GFile *) source);
	g_warning("Error reading '%s': %s", uri, error->message);
	g_free(uri);
}

// GCompareFunc for GNodes in reverse order
static gint
g_node_cmp_rev(GNode *a, GNode *b)
{
	GFile *fa = (GFile *) a->data;
	GFile *fb = (GFile *) b->data;
	g_return_val_if_fail(fa != NULL,  1);
	g_return_val_if_fail(fb != NULL, -1);

	GFileInfo *fia = gel_file_get_file_info(fa);
	GFileInfo *fib = gel_file_get_file_info(fb);
	g_return_val_if_fail(fia != NULL,  1);
	g_return_val_if_fail(fib != NULL, -1);

	GFileType fta = g_file_info_get_file_type(fia);
	GFileType ftb = g_file_info_get_file_type(fib);

	if ((fta == G_FILE_TYPE_DIRECTORY) && (ftb != G_FILE_TYPE_DIRECTORY))
		return  1;
	if ((ftb == G_FILE_TYPE_DIRECTORY) && (fta != G_FILE_TYPE_DIRECTORY))
		return -1;

	return strcmp(g_file_info_get_name(fib),  g_file_info_get_name(fia));
}


