/*
 * lomo/lomo-stats.h
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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __LOMO_STATS_H__
#define __LOMO_STATS_H__

#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define LOMO_TYPE_STATS lomo_stats_get_type()

#define LOMO_STATS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_STATS, LomoStats)) 
#define LOMO_STATS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LOMO_TYPE_STATS, LomoStatsClass)) 
#define LOMO_IS_STATS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_STATS)) 
#define LOMO_IS_STATS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LOMO_TYPE_STATS)) 
#define LOMO_STATS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LOMO_TYPE_STATS, LomoStatsClass))

typedef struct _LomoStatsPrivate LomoStatsPrivate;
typedef struct {
	/*< private >*/
	GObject parent;
	LomoStatsPrivate *priv;
} LomoStats;

typedef struct {
	/*< private >*/
	GObjectClass parent_class;
} LomoStatsClass;

GType lomo_stats_get_type (void);

LomoStats* lomo_stats_new (LomoPlayer *player);

LomoPlayer* lomo_stats_get_player     (LomoStats *self);
gint64      lomo_stats_get_time_played(LomoStats *self);

G_END_DECLS

#endif /* __LOMO_STATS_H__ */

