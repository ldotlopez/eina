/*
 * lomo/lomo-stats.h
 *
 * Copyright (C) 2004-2010 Eina
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <lomo/lomo-player.h>

typedef struct _LomoStats LomoStats;

LomoStats* lomo_stats_watch(LomoPlayer *player);
void       lomo_stats_destroy(LomoStats *stats);

gint64 lomo_stats_get_time_played(LomoStats *stats);

