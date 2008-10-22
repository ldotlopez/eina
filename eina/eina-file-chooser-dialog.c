#define GEL_DOMAIN "Eina::FileChooserDialog"
#include "eina-file-chooser-dialog.h"

#include <glib/gi18n.h>
#include <gel/gel-ui.h>

G_DEFINE_TYPE (EinaFileChooserDialog, eina_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogPrivate))

typedef struct _EinaFileChooserDialogPrivate EinaFileChooserDialogPrivate;

struct _EinaFileChooserDialogPrivate {
	EinaFileChooserDialogAction action;
	GtkBox    *info_box;
	GtkImage  *info_image;
	GtkLabel  *info_label;
	GtkButton *close_button;
};

void
eina_file_chooser_dialog_set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action);

static void
close_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self);

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
	if (G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (eina_file_chooser_dialog_parent_class)->dispose (object);
}

static void
eina_file_chooser_dialog_finalize (GObject *object)
{
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
	eina_file_chooser_dialog_set_action(self, action);

	priv->info_box   = (GtkBox   *) gtk_hbox_new(FALSE, 5);
	priv->info_label = (GtkLabel *) gtk_label_new(NULL);
	priv->info_image = (GtkImage *) gtk_image_new();
	priv->close_button = (GtkButton *) gtk_button_new();

	gtk_button_set_relief(priv->close_button, GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(priv->close_button), gtk_image_new_from_stock("gtk-close", GTK_ICON_SIZE_MENU));

	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(priv->info_image),   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(priv->info_label),   TRUE,  TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->info_box), GTK_WIDGET(priv->close_button), FALSE, FALSE, 0);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));

	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), FALSE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(GTK_DIALOG(self)->vbox), GTK_WIDGET(priv->info_box), 1);

	g_signal_connect(priv->close_button, "clicked",
	G_CALLBACK(close_button_clicked_cb), self);

	return self;
}

void
eina_file_chooser_dialog_set_action(EinaFileChooserDialog *self, EinaFileChooserDialogAction action)
{
	GtkFileFilter *filter;
	GtkButton *queue_button;
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

			queue_button = (GtkButton *) gtk_button_new_with_mnemonic(Q_("_Enqueue"));
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

void eina_file_chooser_dialog_set_msg(EinaFileChooserDialog *self,
	EinaFileChooserDialogMsgType type, gchar *msg)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	switch (type) {
		case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO:
			gtk_image_set_from_stock(priv->info_image, GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
			break;
		case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_WARN:
			gtk_image_set_from_stock(priv->info_image, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
			break;
		case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_ERROR:
			gtk_image_set_from_stock(priv->info_image, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU);
			break;

		case EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_NONE:
		default:
			gtk_widget_hide(GTK_WIDGET(priv->info_box));
			return;
	}
	gtk_label_set_text(GTK_LABEL(priv->info_label), msg);
	gtk_widget_show_all(GTK_WIDGET(priv->info_box));
}

static void
close_button_clicked_cb(GtkWidget *w, EinaFileChooserDialog *self)
{
	EinaFileChooserDialogPrivate *priv = GET_PRIVATE(self);
	gtk_widget_hide(GTK_WIDGET(priv->info_box));
}
