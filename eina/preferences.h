/*
 * eina/preferences.h
 *
 * Copyright (C) 2004-2009 Eina
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

#ifndef _PREFERENCES2_H
#define _PREFERENCES2_H

#include <gel/gel.h>
#include <eina/eina-obj.h>
#include <eina/eina-preferences-dialog.h>

G_BEGIN_DECLS

#define GEL_APP_GET_PREFERENCES(app)  ((EinaPreferencesDialog *) gel_app_shared_get(app, "preferences"))
#define EINA_OBJ_GET_PREFERENCES(obj) GEL_APP_GET_PREFERENCES(eina_obj_get_app(obj))

G_END_DECLS

#endif // _PREFERENCES_H

