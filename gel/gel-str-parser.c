#include <gel/gel-str-parser.h>
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

	for (i = 1; str[i] != '\0'; i+=sizeof(gchar))
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

gchar *
gel_str_parser(gchar *str, GelStrParserFunc callback, gpointer data)
{
	GString *buffer = g_string_new(str), *buffer2;
	gchar *token1, *token2;
	gchar *tmp, *tmp2, *ret;

	while ((token1 = strchr(buffer->str, '{')) != NULL)
	{
		if ((token2 = find_closer(token1)) == NULL)
			goto transform_fail;
		
		// g_printf("Got token: %s\n", token2);

		// Got two tokens
		buffer2 = g_string_new_len(buffer->str, token1 - buffer->str);
		
		tmp = g_strndup(token1 + 1, token2 - token1 - 1);
		if (tmp)
		{
			tmp2 = gel_str_parser(tmp, callback, data);
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

	ret = simple_solver(buffer->str, callback, data);
	// g_printf("Resolving: '%s' => '%s'\n", buffer->str, ret);
	g_string_free(buffer, TRUE);
	return ret;

transform_fail:
	if (buffer != NULL)
		g_string_free(buffer, TRUE);
	return NULL;
}

