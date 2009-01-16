/*
 * tests/eina-tagger.h
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

#ifndef _EINA_TAGGER
#define _EINA_TAGGER

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_TAGGER eina_tagger_get_type()

#define EINA_TAGGER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_TAGGER, EinaTagger))

#define EINA_TAGGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_TAGGER, EinaTaggerClass))

#define EINA_IS_TAGGER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_TAGGER))

#define EINA_IS_TAGGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_TAGGER))

#define EINA_TAGGER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_TAGGER, EinaTaggerClass))

typedef struct {
	GtkEntry parent;
} EinaTagger;

typedef struct {
	GtkEntryClass parent_class;
} EinaTaggerClass;

GType eina_tagger_get_type (void);

EinaTagger* eina_tagger_new (void);

G_END_DECLS

#endif
