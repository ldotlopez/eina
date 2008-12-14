#include <eina/plugin.h>

// --
// Banshee covers
// --
void
coverplus_banshee_search_cb(EinaArtwork *cover, LomoStream *stream, gpointer data)
{
	GString *str;
	gchar *path = NULL;
	gint i, j;
	gchar *input[3] = {
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ARTIST), -1),
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ALBUM), -1),
		NULL
	};

	str = g_string_new(NULL);

	for (i = 0; input[i] != NULL; i++)
	{
		for (j = 0; input[i][j] != '\0'; j++)
		{
			if (g_ascii_isalnum(input[i][j]))
				str = g_string_append_c(str, input[i][j]);
		}
		if (i == 0)
			str = g_string_append_c(str, '-');
		g_free(input[i]);
	}
	str = g_string_append(str, ".jpg");

	path = g_build_filename(g_get_home_dir(), ".config", "banshee", "covers", str->str, NULL);
	g_string_free(str, TRUE);

	if (g_file_test(path, G_FILE_TEST_IS_REGULAR|G_FILE_TEST_EXISTS))
		eina_artwork_provider_success(cover, G_TYPE_STRING, path);
	else
		eina_artwork_provider_fail(cover);

	g_free(path);
}

