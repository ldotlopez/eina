/*
 * eina/player/eina-cover.h
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


#ifndef _EINA_COVER
#define _EINA_COVER

#include <glib-object.h>
#include <gtk/gtk.h>
#include <lomo/lomo-player.h>
#include <eina/art/art.h>

G_BEGIN_DECLS

#define EINA_TYPE_COVER eina_cover_get_type()

#define EINA_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_COVER, EinaCover))

#define EINA_COVER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_COVER, EinaCoverClass))

#define EINA_IS_COVER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_COVER))

#define EINA_IS_COVER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_COVER))

#define EINA_COVER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_COVER, EinaCoverClass))

typedef struct {
	GtkVBox parent;
} EinaCover;

typedef struct {
	GtkVBoxClass parent_class;
} EinaCoverClass;

GType eina_cover_get_type (void);

EinaCover* eina_cover_new (void);
void       eina_cover_set_renderer(EinaCover *self, GtkWidget *renderer);
GtkWidget* eina_cover_get_renderer(EinaCover *self);

void       eina_cover_set_art(EinaCover *self, EinaArt *art);
void       eina_cover_set_lomo_player(EinaCover *self, LomoPlayer *lomo);
void       eina_cover_set_default_pixbuf(EinaCover *self, GdkPixbuf *pixbuf);
void       eina_cover_set_loading_pixbuf(EinaCover *self, GdkPixbuf *pixbuf);

G_END_DECLS

#endif /* _EINA_COVER */
