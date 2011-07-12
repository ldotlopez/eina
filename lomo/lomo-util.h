/*
 * lomo/lomo-util.h
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

#ifndef __LOMO_UTIL_H__
#define __LOMO_UTIL_H__

#include <gst/gst.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

void          lomo_init(gint* argc, gchar ***argv);

gchar    *lomo_create_uri(gchar *str);

gboolean             lomo_format_to_gst(LomoFormat format, GstFormat *gst_format);
gboolean             lomo_state_to_gst(LomoState state, GstState *gst_state);
gboolean             lomo_state_from_gst(GstState state, LomoState *lomo_state);
GstStateChangeReturn lomo_state_change_return_to_gst(GstStateChangeReturn ret);

const gchar *lomo_state_to_str(LomoState state);

const gchar *gst_state_to_str(GstState state);
const gchar *gst_state_change_return_to_str(GstStateChangeReturn s);

/**
 * LOMO_NANOSECS_TO_SECS:
 * @x: nanoseconds
 *
 * Utility macro to convert nanoseconds to secons
 *
 * Returns: Nanoseconds converted to seconds
 **/
#define LOMO_NANOSECS_TO_SECS(x) ((gint64)(x/1000000000L))

/**
 * LOMO_SECS_TO_NANOSECS:
 * @x: Seconds
 *
 * Inverse function of LOMO_NANOSECS_TO_SECS()
 *
 * Returns: Seconds converted to nanoseconds
 **/
#define LOMO_SECS_TO_NANOSECS(x) ((gint64)(x*1000000000L))

G_END_DECLS

#endif // __LOMO_UTIL_H__

