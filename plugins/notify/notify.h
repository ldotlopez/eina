/*
 * plugins/notify/notify.h
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

#ifndef _NOTIFY_H
#define _NOTIFY_H

#include <glib.h>
#include <gel/gel.h>

G_BEGIN_DECLS

typedef struct _EinaNotify EinaNotify;

gboolean eina_notify_init(GelHub *hub, gint argc, gchar *argv[]);
gboolean eina_notify_exit(EinaNotify *self);

G_END_DECLS

#endif // _NOTIFY_H
