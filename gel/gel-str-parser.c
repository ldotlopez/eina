/*
 * gel/gel-str-parser.c
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

#include "gel-str-parser.h"
#include <string.h>

static gchar *
simple_solver(gchar *str, GelStrParserFunc func, gpointer data)
{
	GString *output = g_string_new(NULL);
	gchar *ret = NULL;
	gchar *tag_value = NULL;
	gint i = 0;

	for (i = 0; str[i] != '\0'; i++)
	{
		// Literal '%'
		if ((str[i] == '%') && (str[i+1] == '%'))
		{
			output = g_string_append_c(output, '%');
			i++;
		}

		// Match valid mark
		else if ((str[i] == '%') && (str[i+1] != '\0'))
		{
			if ((tag_value = func(str[i+1], data)) != NULL) {
				output = g_string_append(output, tag_value);
				g_free(tag_value);
				i++;
			}
			else
			{
				output = g_string_append_c(output, '%');
			}
		}

		else
			output = g_string_append_c(output, str[i]);
	}
	ret = output->str;
	g_string_free(output, FALSE);
	return ret;
}

static gchar *
find_closer(gchar *str)
{
	gint tokens = 0;
	gint i;

	for (i = 1; str[i] != '\0'; i += sizeof(gchar))
	{
		switch (str[i])
		{
		case '{':
			tokens++;
			break;

		case '}':
			if (tokens == 0)
				return (gchar *) (str + (i*sizeof(gchar)));
			else
				tokens--;
			break;

		default:
			break;
		}
	}

	return NULL;
}

/**
 * gel_str_parser:
 * @str: Input string
 * @callback: (scope call) (closure user_data): Function to call for each key
 * @user_data: (closure) (allow-none): User data to pass to @callback
 *
 * This function calls @callback for each key found on @str. Each key is
 * substutued by the returned value of @callback. After parsing all the keys
 * the new string is returned
 *
 * Returns: (transfer full): The parsed string
 */
gchar *
gel_str_parser(gchar *str, GelStrParserFunc callback, gpointer user_data)
{
	GString *buffer = g_string_new(str), *buffer2;
	gchar *token1, *token2;
	gchar *tmp, *tmp2, *ret;

	while ((token1 = strchr(buffer->str, '{')) != NULL)
	{
		if ((token2 = find_closer(token1)) == NULL)
			goto transform_fail;

		// Got two tokens
		buffer2 = g_string_new_len(buffer->str, token1 - buffer->str);

		tmp = g_strndup(token1 + 1, token2 - token1 - 1);
		if (tmp)
		{
			tmp2 = gel_str_parser(tmp, callback, user_data);
			if ((tmp2 != NULL) && strcmp(tmp,tmp2))
			{
				buffer2 = g_string_append(buffer2, tmp2);
				g_free(tmp2);
			}
			g_free(tmp);
		}

		buffer2 = g_string_append(buffer2, token2 + 1);

		g_string_free(buffer, TRUE);
		buffer = buffer2;
	}

	ret = simple_solver(buffer->str, callback, user_data);
	g_string_free(buffer, TRUE);
	return ret;

transform_fail:
	if (buffer != NULL)
		g_string_free(buffer, TRUE);
	return NULL;
}

