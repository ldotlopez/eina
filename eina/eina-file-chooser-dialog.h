/* eina-file-chooser-dialog.h */

#ifndef _EINA_FILE_CHOOSER_DIALOG
#define _EINA_FILE_CHOOSER_DIALOG

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_FILE_CHOOSER_DIALOG eina_file_chooser_dialog_get_type()

#define EINA_FILE_CHOOSER_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialog))

#define EINA_FILE_CHOOSER_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogClass))

#define EINA_IS_FILE_CHOOSER_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_FILE_CHOOSER_DIALOG))

#define EINA_IS_FILE_CHOOSER_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_FILE_CHOOSER_DIALOG))

#define EINA_FILE_CHOOSER_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogClass))

typedef struct {
	GtkFileChooserDialog parent;
} EinaFileChooserDialog;

typedef struct {
	GtkFileChooserDialogClass parent_class;
} EinaFileChooserDialogClass;

typedef enum {
	EINA_FILE_CHOOSER_DIALOG_LOAD_FILES
} EinaFileChooserDialogAction;

typedef enum {
	EINA_FILE_CHOOSER_RESPONSE_QUEUE,
	EINA_FILE_CHOOSER_RESPONSE_PLAY
} EinaFileChooserDialogResponse;

typedef enum {
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_NONE,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_WARN,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_ERROR
} EinaFileChooserDialogMsgType;

GType eina_file_chooser_dialog_get_type (void);

EinaFileChooserDialog* eina_file_chooser_dialog_new (EinaFileChooserDialogAction action);
void eina_file_chooser_dialog_set_msg(EinaFileChooserDialog *self,
	EinaFileChooserDialogMsgType type, gchar *msg);

GList *eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self, gboolean recurse);

G_END_DECLS

#endif /* _EINA_FILE_CHOOSER_DIALOG */
