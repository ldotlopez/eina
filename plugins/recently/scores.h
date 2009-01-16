/*
 * plugins/recently/scores.h
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
