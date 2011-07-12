/*
 * lomo/lomo-metadata-parser.h
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

#ifndef _LOMO_METADATA_PARSER_H
#define _LOMO_METADATA_PARSER_H

#include <glib-object.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_TYPE_METADATA_PARSER lomo_metadata_parser_get_type()

#define LOMO_METADATA_PARSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_METADATA_PARSER, LomoMetadataParser)) 
#define LOMO_METADATA_PARSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST  ((klass), LOMO_TYPE_METADATA_PARSER, LomoMetadataParserClass)) 
#define LOMO_IS_METADATA_PARSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_METADATA_PARSER))
#define LOMO_IS_METADATA_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE  ((klass), LOMO_TYPE_METADATA_PARSER))
#define LOMO_METADATA_PARSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj), LOMO_TYPE_METADATA_PARSER, LomoMetadataParserClass))

typedef struct _LomoMetadataParserPrivate LomoMetadataParserPrivate;
typedef struct {
	/* <private> */
	GObject parent;
	LomoMetadataParserPrivate *priv;
} LomoMetadataParser;

typedef struct {
	/* <private> */
	GObjectClass parent_class;

	void (*tag)      (LomoMetadataParser *self, LomoStream *stream, const gchar *tag);
	void (*all_tags) (LomoMetadataParser *self, LomoStream *stream);
} LomoMetadataParserClass;

/**
 * LomoMetadataParserPrio:
 * @LOMO_METADATA_PARSER_PRIO_INVALID: Helper prio, dont use
 * @LOMO_METADATA_PARSER_PRIO_INMEDIATE: Parse stream as soon as possible
 * @LOMO_METADATA_PARSER_PRIO_DEFAULT: Parse stream with default priority
 * @LOMO_METADATA_PARSER_PRIO_N_PRIOS: Helper prio, dont use
 */
typedef enum {
	LOMO_METADATA_PARSER_PRIO_INVALID = -1,
	LOMO_METADATA_PARSER_PRIO_INMEDIATE,
	LOMO_METADATA_PARSER_PRIO_DEFAULT,

	LOMO_METADATA_PARSER_PRIO_N_PRIOS
} LomoMetadataParserPrio;

GType lomo_metadata_parser_get_type (void);

LomoMetadataParser* lomo_metadata_parser_new(void);
void                lomo_metadata_parser_parse(LomoMetadataParser *self, LomoStream *stream, LomoMetadataParserPrio prio);
void                lomo_metadata_parser_clear(LomoMetadataParser *self);

G_END_DECLS

#endif // _LOMO_METADATA_PARSER

