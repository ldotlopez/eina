/*
 * eina/preferences/eina-preferences-dialog.c
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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "eina-preferences-dialog.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaPreferencesDialog, eina_preferences_dialog, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogPrivate))

typedef struct {
	GtkNotebook *notebook;
} EinaPreferencesDialogPrivate;

static void
eina_preferences_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose (object);
}

static void
eina_preferences_dialog_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize (object);
}

static void
eina_preferences_dialog_class_init (EinaPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPreferencesDialogPrivate));

	object_class->dispose = eina_preferences_dialog_dispose;
	object_class->finalize = eina_preferences_dialog_finalize;
}

static void
eina_preferences_dialog_init (EinaPreferencesDialog *self)
{
	EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);

	priv->notebook = GTK_NOTEBOOK(g_object_new(GTK_TYPE_NOTEBOOK,
		"border-width", 12,
		"show-tabs", FALSE,
		NULL));
	gtk_widget_show_all(GTK_WIDGET(priv->notebook));

	gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_box_pack_start(
		GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))),
	 	GTK_WIDGET(priv->notebook),
		TRUE, TRUE, 0);

	gchar *title = g_strdup_printf(_("%s preferences"), PACKAGE_NAME);
	g_object_set((GObject *) self,
		"title", title,
		"destroy-with-parent", TRUE,
		NULL);
	g_free(title);
}

EinaPreferencesDialog*
eina_preferences_dialog_new (GtkWindow *parent)
{
	return EINA_PREFERENCES_DIALOG(g_object_new(EINA_TYPE_PREFERENCES_DIALOG,
		"transient-for", parent,
		NULL));
}

void
eina_preferences_dialog_add_tab(EinaPreferencesDialog *self, EinaPreferencesTab *tab)
{
	g_return_if_fail(EINA_IS_PREFERENCES_DIALOG(self));
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));

	EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	GtkWidget *label_widget = eina_preferences_tab_get_label_widget(tab);

	gtk_widget_show_all(label_widget);
	gtk_widget_show((GtkWidget *) tab);
	gtk_notebook_append_page(priv->notebook, (GtkWidget *) tab, label_widget);

	if (gtk_notebook_get_n_pages(priv->notebook) > 1)
		gtk_notebook_set_show_tabs(priv->notebook, TRUE);
}

void
eina_preferences_dialog_remove_tab(EinaPreferencesDialog *self, EinaPreferencesTab *tab)
{
    g_return_if_fail(EINA_IS_PREFERENCES_DIALOG(self));
    g_return_if_fail(EINA_IS_PREFERENCES_TAB(tab));

	EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	gint n = gtk_notebook_page_num(priv->notebook, (GtkWidget *) tab);
	if (n == -1)
	{
		g_warning(N_("Widget is not on notebook"));
		return;
	}

	gtk_notebook_remove_page(priv->notebook, n);

	if (gtk_notebook_get_n_pages(priv->notebook) <= 1)
		gtk_notebook_set_show_tabs(priv->notebook, FALSE);
}

