/*
 * plugins/fieshta/fieshta-stream.h
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

#ifndef _FIESHTA_STREAM
#define _FIESHTA_STREAM

#include <glib-object.h>
#include <clutter/clutter.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define FIESHTA_STREAM_TYPE_FIESHTA fieshta_stream_get_type()

#define FIESHTA_STREAM_FIESHTA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStream))

#define FIESHTA_STREAM_FIESHTA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamClass))

#define FIESHTA_STREAM_IS_FIESHTA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIESHTA_STREAM_TYPE_FIESHTA))

#define FIESHTA_STREAM_IS_FIESHTA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FIESHTA_STREAM_TYPE_FIESHTA))

#define FIESHTA_STREAM_FIESHTA_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FIESHTA_STREAM_TYPE_FIESHTA, FieshtaStreamClass))

typedef struct {
  ClutterStage parent;
  ClutterActor *cover, *title, *artist;
} FieshtaStream;

typedef struct {
  ClutterStageClass parent_class;
} FieshtaStreamClass;

GType fieshta_stream_get_type (void);

FieshtaStream* fieshta_stream_new (GdkPixbuf *cover, gchar *title, gchar *artist);
void           fieshta_stream_set_cover_from_pixbuf(FieshtaStream* self, GdkPixbuf *cover);

G_END_DECLS

#endif /* _FIESHTA_STREAM */
