#ifndef _SCORES_H
#define _SCORES_H

#include <glib.h>

typedef struct RecentlyScoreRecord {
	guint score;
	gchar *id;
} RecentlyScoreRecord;

RecentlyScoreRecord *
recently_score_record_new(gchar *str);

void
recently_score_record_free(RecentlyScoreRecord *self);

gchar *
recently_score_record_serialize(RecentlyScoreRecord *self);

gint
recently_score_record_compare(RecentlyScoreRecord *a, RecentlyScoreRecord *b);

GList *
recently_score_record_file_read(gchar *filename);

gboolean
recently_score_record_file_write(GList *records, gchar *filename);

GList *
recently_score_record_list_from_hash(GHashTable *input);

#endif // _SCORES_H
