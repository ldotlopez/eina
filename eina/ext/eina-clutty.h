/*
 * eina/ext/eina-clutty.h
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

#ifndef _EINA_CLUTTY_H
#define _EINA_CLUTTY_H

#include <glib-object.h>
#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_CLUTTY eina_clutty_get_type()

#define EINA_CLUTTY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EINA_TYPE_CLUTTY, EinaClutty))

#define EINA_CLUTTY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EINA_TYPE_CLUTTY, EinaCluttyClass))

#define EINA_IS_CLUTTY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EINA_TYPE_CLUTTY))

#define EINA_IS_CLUTTY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EINA_TYPE_CLUTTY))

#define EINA_CLUTTY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EINA_TYPE_CLUTTY, EinaCluttyClass))

typedef struct {
	GtkClutterEmbed parent;
} EinaClutty;

typedef struct {
	GtkClutterEmbedClass parent_class;
} EinaCluttyClass;

GType eina_clutty_get_type (void);

EinaClutty* eina_clutty_new (void);

void  eina_clutty_set_duration(EinaClutty *self, guint duration);
guint eina_clutty_get_duration(EinaClutty *self);

void       eina_clutty_set_pixbuf(EinaClutty *self, GdkPixbuf *pixbuf);
GdkPixbuf *eina_clutty_get_pixbuf(EinaClutty *self);

G_END_DECLS

#endif /* _EINA_CLUTTY_H */
