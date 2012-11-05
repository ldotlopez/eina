/*
 * eina/mpris/eina-mpris-player.h
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

#ifndef __EINA_MPRIS_PLAYER_H__
#define __EINA_MPRIS_PLAYER_H__

#include <glib-object.h>
#include <eina/core/eina-application.h>

G_BEGIN_DECLS

#define EINA_TYPE_MPRIS_PLAYER            eina_mpris_player_get_type()
#define EINA_MPRIS_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayer))
#define EINA_MPRIS_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerClass))
#define EINA_IS_MPRIS_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_MPRIS_PLAYER))
#define EINA_IS_MPRIS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_MPRIS_PLAYER))
#define EINA_MPRIS_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_MPRIS_PLAYER, EinaMprisPlayerClass))

typedef struct _EinaMprisPlayerPrivate EinaMprisPlayerPrivate;
typedef struct {
	GObject parent;
	EinaMprisPlayerPrivate *priv;
} EinaMprisPlayer;

typedef struct {
	GObjectClass parent_class;
} EinaMprisPlayerClass;

GType eina_mpris_player_get_type (void);

EinaMprisPlayer* eina_mpris_player_new (EinaApplication *app, const gchar *bus_name_suffix);

EinaApplication*  eina_mpris_player_get_application    (EinaMprisPlayer *self);
const gchar*      eina_mpris_player_get_bus_name_suffix(EinaMprisPlayer *self);

G_END_DECLS

#endif /* __EINA_MPRIS_PLAYER_H__ */
