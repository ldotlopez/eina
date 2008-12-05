#include <stdlib.h>
#include <gel/gel.h>
#include "scores.h"

RecentlyScoreRecord *
recently_score_record_new(gchar *str)
{
	RecentlyScoreRecord *self = g_new0(RecentlyScoreRecord, 1);
	gchar **tmp, **tmp2;

	if (str == NULL)
		return self;

	tmp = g_strsplit_set(str, "\n\r", 0);
	if ((tmp == NULL) || (tmp[0] == NULL) || (g_str_equal(tmp[0], "")))
	{
		g_strfreev(tmp);
		return self;
	}

	tmp2 = g_strsplit(tmp[0], " ", 2);
	if ((tmp2 == NULL) || \
		(tmp2[0] == NULL) || (g_str_equal(tmp2[0], "")) || \
		(tmp2[1] == NULL) || (g_str_equal(tmp2[1], "")) )
	{
		g_strfreev(tmp2);
		g_strfreev(tmp);
		return self;
	}

	self->score = atoi(tmp2[0]);
	self->id = g_strdup(tmp2[1]);

	g_strfreev(tmp2);
	g_strfreev(tmp);

	return self;
}

void
recently_score_record_free(RecentlyScoreRecord *self)
{
	g_free(self->id);
	g_free(self);
}

gchar *
recently_score_record_serialize(RecentlyScoreRecord *self)
{
	gchar *ret = g_strdup_printf("%d %s", self->score, self->id);
	return ret;
}


gint
recently_score_record_compare(RecentlyScoreRecord *a, RecentlyScoreRecord *b)
{
	if (a->score < b->score) return -1;
	if (a->score > b->score) return  1;
	return 0;
}

GList *
recently_score_record_file_read(gchar *filename)
{
	gchar *buffer = NULL;
	gchar **lines;
	gint  i;
	GList *ret = NULL;

	if (!g_file_get_contents(filename, &buffer, NULL, NULL))
		return NULL;

	lines = g_strsplit_set(buffer, "\n\r", 0);
	for (i = 0; (lines[i] != NULL) && (lines[i][0] != '\0'); i++)
		ret = g_list_prepend(ret, recently_score_record_new(lines[i]));

	ret = g_list_sort(ret, (GCompareFunc) recently_score_record_compare);	

	return g_list_reverse(ret);
}

gboolean
recently_score_record_file_write(GList *records, gchar *filename)
{
	gchar *buffer = NULL;
	GList *scores_serialized = NULL, *iter;

	// Create a new list with serialized records
	iter = records;
	while (iter)
	{
		scores_serialized = g_list_prepend(scores_serialized,
			recently_score_record_serialize((RecentlyScoreRecord *) iter->data));

		iter = iter->next;
	}
	scores_serialized = g_list_reverse(scores_serialized);

	// Join them all into one buffer
	buffer = gel_list_join("\n", scores_serialized);
	gel_list_deep_free(scores_serialized, (GFunc) g_free, NULL);

	if (!g_file_set_contents(filename, buffer, -1, NULL)) {
		g_free(buffer);
		return FALSE;
	}
	g_free(buffer);

	return TRUE;
}

GList *
recently_score_record_list_from_hash(GHashTable *input)
{
	GList *keys, *iter, *ret = NULL;
	RecentlyScoreRecord *record;

	iter = keys = g_hash_table_get_keys(input);
	while (iter)
	{
		record = recently_score_record_new(NULL);
		record->score = GPOINTER_TO_INT(g_hash_table_lookup(input, (gchar *) iter->data));
		record->id = g_strdup((gchar *) iter->data);

		ret = g_list_prepend(ret, record);

		iter = iter->next;
	}
	g_list_free(keys);
	return g_list_sort(ret, (GCompareFunc) recently_score_record_compare);
}


