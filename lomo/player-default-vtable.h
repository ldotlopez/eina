/*
 * lomo/player-default-vtable.h
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

#include <gst/gst.h>
#include <lomo/player.h>

GstElement* default_create(GHashTable *opts, GError **error);
gboolean     default_destroy(GstElement *pipeline, GError **error);

gboolean default_set_stream(GstElement *pipeline, const gchar *uri);

GstStateChangeReturn default_set_state(GstElement *pipeline, GstState state, GError **error);
GstState             default_get_state(GstElement *pipeline);

gboolean default_query_position(GstElement *pipeline, GstFormat *format, gint64 *position);
gboolean default_query_duration(GstElement *pipeline, GstFormat *format, gint64 *duration);

gboolean default_set_volume(GstElement *pipeline, gint volume);
gint     default_get_volume(GstElement *pipeline);

gboolean default_set_mute(GstElement *pipeline, gboolean mute);
gboolean default_get_mute(GstElement *pipeline);

