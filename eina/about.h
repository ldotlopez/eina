/*
 * eina/about.h
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

#ifndef _ABOUT_H
#define _ABOUT_H

G_BEGIN_DECLS

#include <gel/gel.h>
#include <gtk/gtk.h>

#define EINA_ABOUT(self)        ((EinaAbout *) self)
#define gel_app_get_about(app)  EINA_ABOUT(gel_app_shared_get(app, "about"))
#define eina_obj_get_about(obj) gel_app_get_about(eina_obj_get_app(obj))

typedef struct _EinaAbout EinaAbout;

GtkAboutDialog*
eina_about_show(EinaAbout *self);

G_END_DECLS

#endif
