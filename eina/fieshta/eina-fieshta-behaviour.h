/*
 * eina/fieshta/eina-fiestha-behaviour.h
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

#ifndef _EINA_FIESHTA_BEHAVIOUR
#define _EINA_FIESHTA_BEHAVIOUR

#include <glib-object.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_FIESHTA_BEHAVIOUR eina_fieshta_behaviour_get_type()

#define EINA_FIESHTA_BEHAVIOUR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviour)) 
#define EINA_FIESHTA_BEHAVIOUR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST  ((klass), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviourClass)) 
#define EINA_IS_FIESHTA_BEHAVIOUR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR)) 
#define EINA_IS_FIESHTA_BEHAVIOUR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE  ((klass), EINA_TYPE_FIESHTA_BEHAVIOUR)) 
#define EINA_FIESHTA_BEHAVIOUR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj), EINA_TYPE_FIESHTA_BEHAVIOUR, EinaFieshtaBehaviourClass))

typedef struct _EinaFieshtaBehaviourPrivate EinaFieshtaBehaviourPrivate;
typedef struct {
	GObject parent;
	EinaFieshtaBehaviourPrivate *priv;
} EinaFieshtaBehaviour;

typedef struct {
	GObjectClass parent_class;
} EinaFieshtaBehaviourClass;

/**
 * EinaFieshtaBehaviourOption:
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_NONE: Dont block anything
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF: Block forward seek
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW: Block backward seek
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT: Block insert but at the end
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE: Block change but to the next stream
 * @EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT: Default options: SEEK_FF, SEEK_REW,
 *                                         SEEK_INSERT and SEEK_CHANGE
 *
 * Defines the behaviour of an #EinaFieshtaBehaviour
 */
typedef enum {
	EINA_FIESHTA_BEHAVIOUR_OPTION_NONE     = 0,
	EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF  = 1,
	EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW = 1 << 1,
	EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT   = 1 << 2,
	EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE   = 1 << 3,
	EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT  =
		EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_FF  |
		EINA_FIESHTA_BEHAVIOUR_OPTION_SEEK_REW |
		EINA_FIESHTA_BEHAVIOUR_OPTION_INSERT   |
		EINA_FIESHTA_BEHAVIOUR_OPTION_CHANGE
} EinaFieshtaBehaviourOption;

GType eina_fieshta_behaviour_get_type (void);

EinaFieshtaBehaviour* eina_fieshta_behaviour_new (LomoPlayer *lomo, EinaFieshtaBehaviourOption options);

LomoPlayer *
eina_fieshta_behaviour_get_lomo_player(EinaFieshtaBehaviour *self);

EinaFieshtaBehaviourOption
eina_fieshta_behaviour_get_options(EinaFieshtaBehaviour *self);

void
eina_fieshta_behaviour_set_options(EinaFieshtaBehaviour *self, EinaFieshtaBehaviourOption options);

G_END_DECLS

#endif /* _EINA_FIESHTA_BEHAVIOUR */

