/*
 * lomo/player-priv.h
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

#ifndef __LOMO_PLAYER_PRIV_H__
#define __LOMO_PLAYER_PRIV_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	PLAY,
	PAUSE,
	STOP,
	SEEK,
	VOLUME,
	MUTE,
	ADD,
	DEL,
	CHANGE,
	CLEAR,
	REPEAT,
	RANDOM,
	EOS,
	ERROR,
	TAG,
	ALL_TAGS,

	LAST_SIGNAL
} LomoPlayerSignalType;

extern guint lomo_player_signals[LAST_SIGNAL];

G_END_DECLS

#endif
