/*
 * eina/preferences.h
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

#ifndef _PREFERENCES_H
#define _PREFERENCES_H

#include <gel/gel.h>
#include <eina/eina-obj.h>
#include <eina/ext/eina-preferences-dialog.h>

G_BEGIN_DECLS

#define EINA_PREFERENCES(p)           ((EinaPreferences *) p)

#define gel_app_get_preferences(app)  ((EinaPreferences *) gel_app_shared_get(app, "preferences"))
#define eina_obj_get_preferences(obj) gel_app_get_preferences(eina_obj_get_app(obj))

typedef struct _EinaPreferences EinaPreferences;

void
eina_preferences_add_tab_full(EinaPreferences *self, gchar *group, gchar *xml, gchar *root, gchar **objects, guint n,
    GtkImage *icon, GtkLabel *label);

void
eina_preferences_remove_tab(EinaPreferences *self, gchar *group);

G_END_DECLS

#endif // _PREFERENCES_H

