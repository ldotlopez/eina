/*
 * gel/gel-ui-dialogs.c
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

#include "gel-ui-dialogs.h"
#include <glib/gi18n.h>

/**
 * gel_ui_dialog_generic:
 * @parent: (allow-none) (transfer none): Parent window for dialog or %NULL
 * @type: Type of dialog
 * @title: Title for dialog
 * @message: Short message for display
 * @details: (allow-none): Long message for display or %NULL
 * @run_and_destroy: %TRUE if dialog should be run and disposed inmedialty
 *                   or %FALSE if dialog should be returned.
 *
 * Creates a #GtkDialog with the especified parameters. If run_and_destroy
 * is %TRUE the dialog showed and run, %NULL is returned. With run_and_destroy
 * as %FALSE the #GtkWidget is created and returned, but not run.
 *
 * Returns: (transfer full) (allow-none): The dialog or %NULL
 */
GtkWidget *
gel_ui_dialog_generic(GtkWidget *parent, GelUIDialogType type, const gchar *title, const gchar *message, const gchar *details, gboolean run_and_destroy)
{
	if (parent)
		g_return_val_if_fail(GTK_IS_WINDOW(parent), NULL);
	
	g_return_val_if_fail((type >= 0) && (type < GEL_UI_DIALOG_N_TYPES), NULL);
	g_return_val_if_fail(title   != NULL, NULL);
	g_return_val_if_fail(message != NULL, NULL);

	GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
		(GtkWindow *) parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);

	/*
	 * dialog 
	 * `- hbox
	 *    `+- (align) image
	 *     `- vbox
	 *        `+- (align) label-message
	 *         `- expander
	 *            `- label-details
	 */

	// Boxes
	GtkWidget *hbox     = (GtkWidget *) gtk_hbox_new(FALSE, 5);
	GtkWidget *vbox     = (GtkWidget *) gtk_vbox_new(FALSE, 5);

	// Build image with alignment
	const gchar *stock = NULL;
	switch (type)
	{
		case GEL_UI_DIALOG_TYPE_ERROR:
			stock = GTK_STOCK_DIALOG_ERROR;
			break;
		default:
			break;
	}
	g_return_val_if_fail(stock != NULL, NULL);
	GtkWidget *image = (GtkWidget *) gtk_alignment_new(0, 0, 0, 0);
	gtk_container_add(
		(GtkContainer *) image,
		(GtkWidget    *) gtk_image_new_from_stock(stock, GTK_ICON_SIZE_DIALOG));

	// Build message label with alignment
	gchar *tmp = g_strdup_printf("<big><b>%s</b></big>", message);
	GtkWidget *msg = (GtkWidget *) gtk_alignment_new(0, 0, 0, 0);
	GtkWidget *msg_label = (GtkWidget *) gtk_label_new(NULL);
	gtk_label_set_markup((GtkLabel *) msg_label, tmp);
	gtk_container_add((GtkContainer *) msg, msg_label);
	g_free(tmp);

	// Create details if available
	GtkWidget *expander = NULL, *dtls = NULL;
	if (details)
	{
		expander = (GtkWidget *) gtk_expander_new(_("Details"));
		dtls     = (GtkWidget *) gtk_label_new(details);
	}

	gtk_container_add(
		(GtkContainer *) gtk_dialog_get_content_area((GtkDialog *) dialog),
		(GtkWidget *) hbox);

	gtk_box_pack_start((GtkBox *) hbox, image, FALSE, TRUE, 0);
	gtk_box_pack_start((GtkBox *) hbox, vbox,  FALSE, TRUE, 0);

	gtk_box_pack_start((GtkBox *) vbox, msg, FALSE, TRUE, 0);
	if (details)
	{
		gtk_box_pack_start((GtkBox *) vbox, expander, FALSE, TRUE, 0);
		gtk_container_add((GtkContainer *) expander, dtls);

		gtk_label_set_line_wrap((GtkLabel *) dtls, TRUE);
	}

	g_object_set((GObject *) dialog,
		"resizable", FALSE,
		NULL);

	gtk_widget_show_all(dialog);
	if (run_and_destroy)
	{
		gtk_dialog_run((GtkDialog *) dialog);
		gtk_widget_destroy(dialog);
		return NULL;
	}
	else
		return dialog;
}

/**
 * gel_ui_dialog_error:
 * @parent: (allow-none) (transfer none): Parent window for dialog or %NULL
 * @title: Title for dialog
 * @message: Short message for display
 * @details: (allow-none): Long message for display or %NULL
 * @run_and_destroy: %TRUE if dialog should be run and disposed inmedialty
 *                   or %FALSE if dialog should be returned.
 *
 * Calls gel_ui_dialog_generic() with type = %GEL_UI_DIALOG_TYPE_ERROR
 *
 * Returns: (transfer full) (allow-none): The dialog or %NULL
 */
GtkWidget *
gel_ui_dialog_error(GtkWidget *parent, const gchar *title, const gchar *message, const gchar *details, gboolean run_and_destroy)
{
	return gel_ui_dialog_generic(parent, GEL_UI_DIALOG_TYPE_ERROR, title, message, details, run_and_destroy);
}

