/*
 * eina/ext/eina-plugin-dialog.h
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

#ifndef _EINA_PLUGIN_DIALOG
#define _EINA_PLUGIN_DIALOG

#include <gel/gel.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLUGIN_DIALOG eina_plugin_dialog_get_type()

#define EINA_PLUGIN_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialog))

#define EINA_PLUGIN_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogClass))

#define EINA_IS_PLUGIN_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLUGIN_DIALOG))

#define EINA_IS_PLUGIN_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PLUGIN_DIALOG))

#define EINA_PLUGIN_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogClass))

typedef struct {
	GtkDialog parent;
} EinaPluginDialog;

typedef struct {
	GtkDialogClass parent_class;
} EinaPluginDialogClass;

enum {
	EINA_PLUGIN_DIALOG_RESPONSE_INFO = 1
};

GType eina_plugin_dialog_get_type (void);

EinaPluginDialog*
eina_plugin_dialog_new (GelApp *app);

GelApp *
eina_plugin_dialog_get_app(EinaPluginDialog *self);

GelPlugin *
eina_plugin_dialog_get_selected_plugin(EinaPluginDialog *self);


G_END_DECLS

#endif
