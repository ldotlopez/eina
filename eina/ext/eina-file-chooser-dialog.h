/*
 * eina/ext/eina-file-chooser-dialog.h
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

#ifndef __EINA_FILE_CHOOSER_DIALOG_H__
#define __EINA_FILE_CHOOSER_DIALOG_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_FILE_CHOOSER_DIALOG eina_file_chooser_dialog_get_type()
#define EINA_FILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialog))
#define EINA_FILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogClass))
#define EINA_IS_FILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_FILE_CHOOSER_DIALOG))
#define EINA_IS_FILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_FILE_CHOOSER_DIALOG))
#define EINA_FILE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_FILE_CHOOSER_DIALOG, EinaFileChooserDialogClass))

typedef struct _EinaFileChooserDialogPrivate EinaFileChooserDialogPrivate;
typedef struct {
	GtkFileChooserDialog parent;
	EinaFileChooserDialogPrivate *priv;
} EinaFileChooserDialog;

typedef struct {
	GtkFileChooserDialogClass parent_class;
} EinaFileChooserDialogClass;

/**
 * EinaFileChooserDialogAction:
 * @EINA_FILE_CHOOSER_DIALOG_ACTION_LOAD_FILES: Filechooser dialog will be used to
 *                                       load files
 *
 * Possible actions for the filechooser dialog
 */
typedef enum {
	EINA_FILE_CHOOSER_DIALOG_ACTION_LOAD_FILES
} EinaFileChooserDialogAction;

/**
 * EinaFileChooserDialogResponse:
 * @EINA_FILE_CHOOSER_RESPONSE_QUEUE: URIs were selected for queue
 * @EINA_FILE_CHOOSER_RESPONSE_PLAY: URIs were selected for play
 *
 * Possible responses from the filechooser dialog
 */
typedef enum {
	EINA_FILE_CHOOSER_RESPONSE_QUEUE = 1,
	EINA_FILE_CHOOSER_RESPONSE_PLAY
} EinaFileChooserDialogResponse;

/**
 * EinaFileChooserDialogMsgType:
 * @EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_NONE: No type
 * @EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO: Information message
 * @EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_WARN: Warning message
 * @EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_ERROR: Error message
 *
 * Possible messages types, each message type uses diferent visual indicators,
 * p.ex. icons.
 */
typedef enum {
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_NONE,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_WARN,
	EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_ERROR
} EinaFileChooserDialogMsgType;

GType eina_file_chooser_dialog_get_type (void);

EinaFileChooserDialog* eina_file_chooser_dialog_new (GtkWindow *parent, EinaFileChooserDialogAction action);
void eina_file_chooser_dialog_set_msg(EinaFileChooserDialog *self,
	EinaFileChooserDialogMsgType type, const gchar *msg);

GList *eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self);

G_END_DECLS

#endif /* __EINA_FILE_CHOOSER_DIALOG_H__ */
