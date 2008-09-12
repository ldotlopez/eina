#ifndef _GEL_STR_PARSER_H
#define _GEL_STR_PARSER_H

#include <glib.h>

typedef gchar* (*GelStrParserFunc)(gchar key, gpointer data);
gchar *gel_str_parser(gchar *str, GelStrParserFunc callback, gpointer data);

#endif // _GEL_STR_PARSER_H
