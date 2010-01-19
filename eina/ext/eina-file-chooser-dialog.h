/*
 * eina/ext/eina-file-chooser-dialog.h
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
	EINA_FILE_CHOOSER_RESPONSE_QUEUE = 1,
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

GSList *eina_file_chooser_dialog_get_uris(EinaFileChooserDialog *self);


G_END_DECLS

#endif /* _EINA_FILE_CHOOSER_DIALOG */
