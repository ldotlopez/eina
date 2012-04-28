/*
 * eina/dock/eina-dock-plugin.h
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

#ifndef __EINA_DOCK_PLUGIN_H__
#define __EINA_DOCK_PLUGIN_H__

#include <eina/dock/eina-dock.h>
#include <eina/ext/eina-application.h>

G_BEGIN_DECLS

EinaDock* eina_application_get_dock(EinaApplication *application);

EinaDockTab* eina_application_add_dock_widget   (EinaApplication *application,
	const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlag flags);
gboolean     eina_application_switch_dock_widget(EinaApplication *application, EinaDockTab *tab);
gboolean     eina_application_remove_dock_widget(EinaApplication *application, EinaDockTab *tab);

#define EINA_DOCK_PREFERENCES_DOMAIN EINA_APP_DOMAIN".preferences.dock"
#define EINA_DOCK_ORDER_KEY          "page-order"
#define EINA_DOCK_EXPANDED_KEY       "expanded"
#define EINA_DOCK_WINDOW_H_KEY       "window-height"
#define EINA_DOCK_WINDOW_W_KEY       "window-width"

G_END_DECLS

#endif // __EINA_DOCK_PLUGIN_H__

