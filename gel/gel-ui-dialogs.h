/*
 * gel/gel-ui-dialogs.h
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

#include <gel/gel-ui.h>

#ifndef _GEL_UI_DIALOGS
#define _GEL_UI_DIALOGS

G_BEGIN_DECLS

typedef enum {
	GEL_UI_DIALOG_TYPE_ERROR = 0,
	GEL_UI_DIALOG_N_TYPES
} GelUIDialogType;

GtkWidget *
gel_ui_dialog_generic(GtkWidget *parent,
	GelUIDialogType type, const gchar *title, const gchar *message, const gchar *details, gboolean run_and_destroy);

GtkWidget *
gel_ui_dialog_error(GtkWidget *parent,
	const gchar *title, const gchar *message, const gchar *details, gboolean run_and_destroy);


G_END_DECLS

#endif

