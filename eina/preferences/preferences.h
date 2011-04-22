/*
 * eina/preferences/preferences.h
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

#ifndef _PREFERENCES_H
#define _PREFERENCES_H

#include <eina/eina-plugin.h>
#include <eina/preferences/eina-preferences-dialog.h>

G_BEGIN_DECLS

typedef struct _EinaPreferences EinaPreferences;

#define eina_application_get_preferences(engine) ((EinaPreferences *) eina_application_get_interface(engine, "preferences"))
#define eina_plugin_get_preferences(plugin)      eina_application_get_preferences(eina_plugin_get_application(plugin))


void
eina_preferences_add_tab(EinaPreferences *self, EinaPreferencesTab *tab);

void
eina_preferences_remove_tab(EinaPreferences *self, EinaPreferencesTab *tab);

G_END_DECLS

#endif // _PREFERENCES_H

