/*
 * plugins/player/eina-player.h
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

#ifndef _EINA_PLAYER
#define _EINA_PLAYER

#include <glib-object.h>
#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLAYER eina_player_get_type()

#define EINA_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLAYER, EinaPlayer))

#define EINA_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PLAYER, EinaPlayerClass))

#define EINA_IS_PLAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLAYER))

#define EINA_IS_PLAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PLAYER))

#define EINA_PLAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PLAYER, EinaPlayerClass))

typedef struct {
	GelUIGeneric parent;
} EinaPlayer;

typedef struct {
	GelUIGenericClass parent_class;
} EinaPlayerClass;

GType eina_player_get_type (void);

GtkWidget* eina_player_new (void);

void
eina_player_set_lomo_player(EinaPlayer *self, LomoPlayer *lomo);

G_END_DECLS

#endif /* _EINA_PLAYER */

