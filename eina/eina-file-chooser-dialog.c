#define GEL_DOMAIN "Eina::FileChooserDialog"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <gel/gel-ui.h>
#include <eina/eina-file-chooser-dialog.h>

G_DEFINE_TYPE (EinaFileChooserDialog, eina_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogPrivate))

typedef struct _EinaFileChooserDialogPrivate EinaFileChooserDialogPrivate;

struct _EinaFileChooserDialogPrivate {
	EinaFileChooserDialogAction action;
	GtkBox *info_box;
	GSList *uris;

	GQueue       *queue;
	GCancellable *cancellable;
	GelIOOp      *op;
};

void
set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action);

static void
update_sensitiviness(EinaFileChooserDialog *self, gboolean value);
static void
clear_box(EinaFileChooserDialog *self);
static void
run_queue(EinaFileChooserDialog *self);

// Async ops
static void
file_chooser_query_info_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
recurse_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
static void
recurse_read_error_cb(GelIOOp *op, GFile *f, GError *error, gpointer data);
static void
recurse_tree_parse(GelIORecurseTree *tree, GFile *f, EinaFileChooserDialog *self);

static void
close_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self);
static void
cancel_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self);

static void
reset_resources(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	if (!g_queue_is_empty(priv->queue))
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

	if (priv->cancellable)
	{
		gel_warn("Cancelling via cancellable");
		if (!g_cancellable_is_cancelled(priv->cancellable))
			g_cancellable_cancel(priv->cancellable);
		g_object_unref(priv->cancellable);
		priv->cancellable = NULL;
	}

	if (priv->op)
	{
		gel_warn("Cancelling via GelIOOp");
		gel_io_op_cancel(priv->op);
		gel_io_op_unref(priv->op);
		priv->op = NULL;
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
#if 0
	e_info("%d properties:", n_properties);
	for (i = 0 ; i < n_properties; i++)
	{
		e_info("[%s]  %s: %s",
			G_VALUE_TYPE_NAME(properties[i].value), properties[i].pspec->name, g_strdup_value_contents(properties[i].value));
	}
#endif

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

	if (!gel_ui_stock_add("queue.png", "eina-queue", GTK_ICON_SIZE_MENU, NULL))
		gel_error("Cannot find and add to stock file queue.png");
}

static void
eina_file_chooser_dialog_init (EinaFileChooserDialog *self)
{
}

EinaFileChooserDialog*
eina_file_chooser_dialog_new (EinaFileChooserDialogAction action)
{
	EinaFileChooserDialog *self;
	EinaFileChooserDialogPrivate *priv;

	self = g_object_new (EINA_TYPE_FILE_CHOOSER_DIALOG, NULL);
	priv = GET_PRIVATE(self);
	priv->action = action;

	g_object_set(G_OBJECT(self), "local-only", FALSE, NULL);
	set_action(self, action);

	priv->queue = g_queue_new();

	priv->info_box   = (GtkBox   *) gtk_hbox_new(FALSE, 5);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));

	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), FALSE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), 1);

	return self;
}

void eina_file_chooser_dialog_set_custom_msg(EinaFileChooserDialog *self,
	GtkImage *image, GtkLabel *label, GtkWidget *action)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	clear_box(self);

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
	g_signal_connect(button, "clicked",
	G_CALLBACK(close_button_clicked_cb), self);

	eina_file_chooser_dialog_set_custom_msg(self,
		(GtkImage *) gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU),
		(GtkLabel *) gtk_label_new(msg),
		GTK_WIDGET(button)
		);
}

gint
eina_file_chooser_dialog_run(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	gint response;
	GSList *uris;

	reset_resources(self);

	if (priv->action != EINA_FILE_CHOOSER_DIALOG_LOAD_FILES)
	{
		gel_warn("EinaFileChooserDialog dont know how to run in mode %d", priv->action);
		return GTK_RESPONSE_NONE;
	}

	switch(response = gtk_dialog_run(GTK_DIALOG(self)))
	{
	case EINA_FILE_CHOOSER_RESPONSE_PLAY:
	case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
		uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(self));
		if (uris == NULL)
			break;

		// Lock UI
		update_sensitiviness(self, FALSE);

		// Loading image
		gchar *loading_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, "file-chooser-loading.gif");
		GtkImage *loading = (GtkImage *) gtk_image_new_from_file(loading_path);
		g_free(loading_path);

		// The 'cancel' button
		GtkButton *cancel = (GtkButton *) gtk_button_new();
		gtk_button_set_relief(cancel, GTK_RELIEF_NONE);
		gtk_container_add(GTK_CONTAINER(cancel), gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU));
		g_signal_connect(cancel, "clicked",
		G_CALLBACK(cancel_button_clicked_cb), self);
		
		// Show busy UI
		eina_file_chooser_dialog_set_custom_msg(self,
			loading,
			(GtkLabel *) gtk_label_new(_("Loading files...")),
			GTK_WIDGET(cancel));

		// Start processing
		
		priv->queue = g_slist_sort(uris, (GCompareFunc) strcmp);
		run_queue(self);

		gtk_main(); // Re-enter main loop

	default:
		break;
	}

	return response;
}

GSList *
eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	return g_slist_copy(priv->uris);
}

// --
// Internal
// --
void
set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action)
{
	GtkFileFilter *filter;
	const gchar *music_folder;

	music_folder = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);

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

		GtkButton *queue_button = (GtkButton *) gtk_button_new_with_mnemonic(Q_("_Enqueue"));
		gtk_button_set_image(queue_button, gtk_image_new_from_stock("eina-queue", GTK_ICON_SIZE_BUTTON));
		gtk_widget_show_all(GTK_WIDGET(queue_button));

		gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_MEDIA_PLAY, EINA_FILE_CHOOSER_RESPONSE_PLAY);
		gtk_dialog_add_action_widget(GTK_DIALOG(self), GTK_WIDGET(queue_button), EINA_FILE_CHOOSER_RESPONSE_QUEUE);
		gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

		gtk_window_set_title(GTK_WINDOW(self), _("Enqueue files"));
		break;

	default:
		gel_error("EinaFileChooserDialog unknow action %d", action);
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
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(self)), NULL);
		g_signal_handlers_disconnect_by_func(self, gtk_true, self);
	}
	else
	{
		GtkWidget *w = GTK_WIDGET(self);
		GdkWindow *win = gtk_widget_get_window(w);
		GdkDisplay *display = gtk_widget_get_display(w);
		gdk_window_set_cursor(win, gdk_cursor_new_for_display(display, GDK_WATCH));
		g_signal_connect(self, "delete-event", G_CALLBACK(gtk_true), self);
	}
}

static void
clear_box(EinaFileChooserDialog *self)
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
		gtk_main_quit();
		return;
	}

	// Process next element
	gchar *uri = g_queue_push_head(priv->queue);

	if (priv->cancellable)
	{
		if (!g_cancellable_is_cancelled(priv->cancellable))
		{
			gel_warn("run_queue called but GCancellable is not cancelled");
			g_cancellable_cancel(priv->cancellable);
		}
		g_object_unref(priv->cancellable);
	}

	priv->cancellable = g_cancellable_new();
	g_file_query_info_async(g_file_new_for_uri(uri), "standard::*",
		0, G_PRIORITY_DEFAULT, priv->cancellable,
		file_chooser_query_info_cb, self);
	g_free(uri);
}

// --
// Handle async reads
// --

// Query GFileInfo for URI and add or recurse over dir
static void
file_chooser_query_info_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	EinaFileChooserDialog *self = EINA_FILE_CHOOSER_DIALOG(data);
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	GFile *file = G_FILE(source);
	GError *err = NULL;

	// Finish operation
	GFileInfo *info = g_file_query_info_finish(file, res, &err);
	g_object_unref(priv->cancellable);
	priv->cancellable = NULL;

	if (err != NULL)
	{
		gchar *uri = g_file_get_uri(file);
		g_object_unref(source);
		gel_warn("Cannot fetch info for '%s': %s", uri, err->message);
		g_free(uri);
		g_error_free(err);
	}

	if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
		priv->op = gel_io_recurse_dir(file, "standard::*",
			recurse_read_success_cb, recurse_read_error_cb, self);
	else
		priv->uris = g_slist_prepend(priv->uris, g_file_get_uri(file)); // Share memory

	g_object_unref(source);
	g_object_unref(info);
}

// Walk over GelIORecurseTree and add files
static void
recurse_read_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data)
{
	EinaFileChooserDialog *self = EINA_FILE_CHOOSER_DIALOG(data);
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	// Get and parse result
	GelIORecurseTree *tree = gel_io_op_result_get_recurse_tree(result);
	recurse_tree_parse(tree, gel_io_recurse_tree_get_root(tree), EINA_FILE_CHOOSER_DIALOG(data));

	// Close this operation and go to next element in queue
	gel_io_op_unref(op);
	priv->op = NULL;

	run_queue(self);
}

// Show errors from GelIOOp and update UI to reflect them
static void
recurse_read_error_cb(GelIOOp *op, GFile *f, GError *error, gpointer data)
{
	gchar *uri = g_file_get_uri(f);
	gel_error("Error reading '%s': %s", uri, error->message);
	g_free(uri);

	// remove_op(EINA_FILE_CHOOSER_DIALOG(data), op);
	// gel_io_op_unref(op); <-- not sure
	// run_queue(self);
}

// Walk over RecurseTree
static void
recurse_tree_parse(GelIORecurseTree *tree, GFile *f, EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);

	if (f == NULL)
		return;

	GList *children = gel_io_recurse_tree_get_children(tree, f);
	GList *iter = children;

	while (iter)
	{
		GFileInfo *info = G_FILE_INFO(iter->data);
		GFile *child = gel_io_file_get_child_for_file_info(f, info);

		if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
			recurse_tree_parse(tree, child, self);
		else
			priv->uris = g_slist_prepend(priv->uris, g_file_get_uri(child)); // Shared

		g_object_unref(child);
		iter = iter->next;
	}

	g_list_free(children);
}

static void
close_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));
}

static void
cancel_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self)
{
	gel_warn("Cancel must go on!");
	internal_reset(self);
}

