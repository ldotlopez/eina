/*
 * eina/muine/eina-muine.h
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

#ifndef _EINA_MUINE
#define _EINA_MUINE

#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>
#include <eina/adb/eina-adb.h>

G_BEGIN_DECLS

#define EINA_TYPE_MUINE eina_muine_get_type()

#define EINA_MUINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_MUINE, EinaMuine))
#define EINA_MUINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_MUINE, EinaMuineClass))
#define EINA_IS_MUINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_MUINE))
#define EINA_IS_MUINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_MUINE))
#define EINA_MUINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_MUINE, EinaMuineClass))

typedef struct _EinaMuinePrivate EinaMuinePrivate;
typedef struct {
	GelUIGeneric      parent;
	EinaMuinePrivate *priv;
} EinaMuine;

typedef struct {
	GelUIGenericClass parent_class;
} EinaMuineClass;

typedef enum {
	EINA_MUINE_MODE_INVALID = -1,
	EINA_MUINE_MODE_ALBUM   = 0,
	EINA_MUINE_MODE_ARTIST
} EinaMuineMode;

GType eina_muine_get_type (void);

EinaMuine* eina_muine_new (void);

void
eina_muine_set_adb(EinaMuine *self, EinaAdb *adb);
EinaAdb*
eina_muine_get_adb(EinaMuine *self);

void
eina_muine_set_lomo_player(EinaMuine *self, LomoPlayer *lomo);
LomoPlayer*
eina_muine_get_lomo_player(EinaMuine *lomo);

void
eina_muine_set_mode(EinaMuine *self, EinaMuineMode mode);
EinaMuineMode
eina_muine_get_mode(EinaMuine *self);


G_END_DECLS

#endif /* _EINA_MUINE */

