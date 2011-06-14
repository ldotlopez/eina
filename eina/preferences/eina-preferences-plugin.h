/*
 * eina/preferences/eina-preferences-plugin.h
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
 * MERCHANTABILITY or FITNESS FOR A PPREFERENCESICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __EINA_PREFERENCES_PLUGIN_H__
#define __EINA_PREFERENCES_PLUGIN_H__

#include <eina/ext/eina-extension.h>
#include <eina/preferences/eina-preferences-dialog.h>

G_BEGIN_DECLS

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_PREFERENCES_PLUGIN         (eina_preferences_plugin_get_type ())
#define EINA_PREFERENCES_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPlugin))
#define EINA_PREFERENCES_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPlugin))
#define EINA_IS_PREFERENCES_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_PREFERENCES_PLUGIN))
#define EINA_IS_PREFERENCES_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_PREFERENCES_PLUGIN))
#define EINA_PREFERENCES_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_PREFERENCES_PLUGIN, EinaPreferencesPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaPreferencesPlugin, eina_preferences_plugin)

/**
 * EinaApplication accessors
 */
typedef struct _EinaPreferences EinaPreferences;

EinaPreferences *
eina_application_get_preferences(EinaApplication *application);
void
eina_application_add_preferences_tab(EinaApplication *application, EinaPreferencesTab *tab);
void
eina_application_remove_preferences_tab(EinaApplication *application, EinaPreferencesTab *tab);

/**
 * EinaPrefences API
 */
void
eina_preferences_add_tab (EinaPreferences *prefences, EinaPreferencesTab *tab);
void
eina_preferences_remove_tab(EinaPreferences *prefences, EinaPreferencesTab *tab);




G_END_DECLS

#endif

