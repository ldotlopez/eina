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

#include <eina/core/eina-extension.h>
#include <eina/preferences/eina-preferences-dialog.h>

G_BEGIN_DECLS

void eina_application_add_preferences_tab   (EinaApplication *application, EinaPreferencesTab *tab);
void eina_application_remove_preferences_tab(EinaApplication *application, EinaPreferencesTab *tab);

G_END_DECLS

#endif

