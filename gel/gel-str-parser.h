/*
 * gel/gel-str-parser.h
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

#ifndef _GEL_STR_PARSER_H
#define _GEL_STR_PARSER_H

#include <glib.h>

typedef gchar* (*GelStrParserFunc)(gchar key, gpointer data);
gchar *gel_str_parser(gchar *str, GelStrParserFunc callback, gpointer data);

#endif // _GEL_STR_PARSER_H
