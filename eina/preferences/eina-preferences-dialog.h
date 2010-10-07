/*
 * eina/ext/eina-preferences-dialog.h
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

#ifndef _EINA_PREFERENCES_DIALOG
#define _EINA_PREFERENCES_DIALOG

#include <gtk/gtk.h>
#include <eina/preferences/eina-preferences-tab.h>

G_BEGIN_DECLS

#define EINA_TYPE_PREFERENCES_DIALOG eina_preferences_dialog_get_type()

#define EINA_PREFERENCES_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialog))

#define EINA_PREFERENCES_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogClass))

#define EINA_IS_PREFERENCES_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PREFERENCES_DIALOG))

#define EINA_IS_PREFERENCES_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PREFERENCES_DIALOG))

#define EINA_PREFERENCES_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogClass))

typedef struct {
	GtkDialog parent;
} EinaPreferencesDialog;

typedef struct {
	GtkDialogClass parent_class;
} EinaPreferencesDialogClass;

GType eina_preferences_dialog_get_type (void);

EinaPreferencesDialog*
eina_preferences_dialog_new (void);

void
eina_preferences_dialog_add_tab(EinaPreferencesDialog *self, EinaPreferencesTab *tab);

void
eina_preferences_dialog_remove_tab(EinaPreferencesDialog *self, EinaPreferencesTab *tab);

G_END_DECLS

#endif
