#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>

const gchar *stream_get_tag(gchar abbr)
{
	if (abbr == 'a')
		return "Artist";
	else if (abbr == 'b')
		return "Album";
	else if (abbr == 't')
		return "Title";
	else
		return NULL;
}

// Resolve a simple string, no conditionals.
// Returns a newly allocated string
gchar *resolver_string(gchar *str)
{
	GString *output = g_string_new(NULL);
	gchar *ret = NULL;
	const gchar *tag_value = NULL;
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
			if ((tag_value = stream_get_tag(str[i+1])) != NULL) {
				output = g_string_append(output, tag_value);
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

// Finds the closer of the current open token
// Returns a pointer to the end token
gchar *
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


gchar *decondition_string(gchar *str)
{
	gchar *ret = NULL;
	GString *buff = g_string_new(NULL);
	gchar *token1, *token2;
	gchar *tmp, *tmp_parsed;

	while ((token1 = strchr(str, '{')) != NULL)
	{
		token2 = find_closer(token1);

		// Broken format
		if (token2 == NULL)
		{
			g_string_free(buff, TRUE);
			return NULL;
		}

		// Copy pre-token1
		buff = g_string_append_len(buff, str, token1 - str);

		// Try to resolve conditional part
		tmp = g_strndup(token1+1, token2 - token1);
		if (tmp) 
		{
			tmp_parsed = resolver_string(tmp);
			if (tmp_parsed)
			{
				buff = g_string_append(buff, tmp_parsed);
				g_free(tmp_parsed);
			}
			g_free(tmp);
		}

		// Copy post-token2
		buff = g_string_append(buff, token2+1);

		str = token2+1;
	}

	ret = buff->str;
	g_string_free(buff, FALSE);
	return ret;
}

gchar *
resolver_string_with_conditional(gchar *str)
{
	gchar *decond = decondition_string(str);
	gchar *ret  = resolver_string(decond);
	g_free(decond);
	return ret;
#if 0
	gchar *token1, *token2;
	GString *output = g_string_new(NULL);
	gchar *tmp;
	gchar *tmp_parsed;
	gchar *decond, *ret;

	while ((token1 = strchr(str, '{')) != NULL)
	{
		token2 = find_closer(token1);

		// Broken format
		if (token2 == NULL)
			break;

		output = g_string_append_len(output, str, token1 - str);
		tmp = g_strndup(token1+1, token2 - token1);
		if (tmp) 
		{
			tmp_parsed = resolver_string(tmp);
			if (tmp_parsed)
			{
				output = g_string_append(output, tmp_parsed);
				g_free(tmp_parsed);
			}
			g_free(tmp);
		}

		output = g_string_append(output, token2+1);

		decond = output->str;
		g_string_free(output, FALSE);
	}

	else
	{
		decond = g_strdup(str);
	}

	ret = resolver_string(decond);
	g_free(decond);
#endif
}
gint main(gint argc, gchar *argv[])
{
	gchar *fmt_str = argv[1];
	gchar *out = resolver_string_with_conditional(fmt_str);
	g_printf("%s => %s\n", fmt_str, out);
	g_free(out);
	return 0;
}

