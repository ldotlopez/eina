/*
 * lomo/lomo-player.h
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

#ifndef __LOMO_PLAYER_H
#define __LOMO_PLAYER_H

#include <glib-object.h>
#include <gst/gst.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_TYPE_PLAYER         (lomo_player_get_type ())
#define LOMO_PLAYER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), LOMO_TYPE_PLAYER, LomoPlayer))
#define LOMO_PLAYER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), LOMO_TYPE_PLAYER, LomoPlayerClass))
#define LOMO_IS_PLAYER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), LOMO_TYPE_PLAYER))
#define LOMO_IS_PLAYER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), LOMO_TYPE_PLAYER))
#define LOMO_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), LOMO_TYPE_PLAYER, LomoPlayerClass))

typedef enum {
	LOMO_STATE_CHANGE_SUCCESS     = GST_STATE_CHANGE_SUCCESS,
	LOMO_STATE_CHANGE_ASYNC       = GST_STATE_CHANGE_ASYNC,
	LOMO_STATE_CHANGE_NO_PREROLL  = GST_STATE_CHANGE_NO_PREROLL,
	LOMO_STATE_CHANGE_FAILURE     = GST_STATE_CHANGE_FAILURE,
} LomoStateChangeReturn;

typedef enum {
	LOMO_STATE_INVALID = -1,
	LOMO_STATE_STOP    = 0,
	LOMO_STATE_PLAY    = 1,
	LOMO_STATE_PAUSE   = 2,
} LomoState;

typedef enum {
	LOMO_FORMAT_INVALID = -1,
	LOMO_FORMAT_TIME    = 0,
	LOMO_FORMAT_PERCENT = 1,

	LOMO_FORMATS
} LomoFormat;

typedef struct _LomoPlayerPrivate LomoPlayerPrivate;
typedef struct
{
	GObject parent;

	LomoPlayerPrivate *priv;
} LomoPlayer;

typedef struct
{
	GObjectClass parent_class;

	void (*play)     (LomoPlayer *self);
	void (*pause)    (LomoPlayer *self);
	void (*stop)     (LomoPlayer *self);
	void (*seek)     (LomoPlayer *self, gint64 old, gint64 new);
	void (*volume)   (LomoPlayer *self, gint volume);
	void (*mute)     (LomoPlayer *self, gboolean mute);
	void (*add)      (LomoPlayer *self, LomoStream *stream, gint pos);
	void (*del)      (LomoPlayer *self, gint pos);
	void (*change)   (LomoPlayer *self, gint from, gint to);
	void (*clear)    (LomoPlayer *self);
	void (*repeat)   (LomoPlayer *self, gboolean val);
	void (*random)   (LomoPlayer *self, gboolean val);
	void (*eos)      (LomoPlayer *self);
	void (*error)    (LomoPlayer *self, GError *error);
	void (*tag)      (LomoPlayer *self, LomoStream *stream, LomoTag tag);
	void (*all_tags) (LomoPlayer *self, LomoStream *stream);
} LomoPlayerClass;
GType lomo_player_get_type(void);

typedef struct {
	GstElement* (*create_pipeline)  (const gchar *uri, GHashTable *opts);
	void        (*destroy_pipeline) (GstElement *pipeline);

	GstStateChangeReturn (*set_state) (GstElement *pipeline, GstState state);
	GstState             (*get_state) (GstElement *pipeline);

	gboolean (*set_position) (GstElement *pipeline, GstFormat  format, gint64  position);
	gboolean (*get_position) (GstElement *pipeline, GstFormat *format, gint64 *position);
	gboolean (*get_length)   (GstElement *pipeline, GstFormat *format, gint64 *duration);

	// 0 lowest, 100 highest (there is not a common range over all posible
	// sinks, so make it relative and let vfunc set it
	gboolean (*set_volume) (GstElement *pipeline, gint volume);
	gint     (*get_volume) (GstElement *pipeline);

	// Can be omited
	gboolean (*set_mute) (GstElement *pipeline, gboolean mute);
	gboolean (*get_mute) (GstElement *pipeline);
} LomoPlayerVTable;

#define lomo_init(argc,argv)    gst_init(argc,argv)
#define lomo_get_option_group() gst_init_get_option_group()

LomoPlayer       *lomo_player_new(const gchar *option_name, ...);

gboolean    lomo_player_reset(LomoPlayer *self, GError **error); // API Changed
LomoStream *lomo_player_get_stream(LomoPlayer *self);

// Quick play functions, simple shortcuts.
gboolean lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error); // API Changed
gboolean lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error); // API Changed

// Get/Set state
LomoStateChangeReturn lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error);
#define lomo_player_play(p,error)  lomo_player_set_state(p,LOMO_STATE_PLAY, error)
#define lomo_player_pause(p,error) lomo_player_set_state(p,LOMO_STATE_PAUSE,error)
#define lomo_player_stop(p,error)  lomo_player_set_state(p,LOMO_STATE_STOP, error)
LomoState lomo_player_get_state(LomoPlayer *self);

// Seek/Query
gint64 lomo_player_tell(LomoPlayer *self, LomoFormat format);
#define lomo_player_tell_time(p)    lomo_player_tell(p,LOMO_FORMAT_TIME)
#define lomo_player_tell_percent(p) lomo_player_tell(p,LOMO_FORMAT_PERCENT)

gboolean lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val);
#define lomo_player_seek_time(c,t)    lomo_player_seek(c,LOMO_FORMAT_TIME,t)
#define lomo_player_seek_percent(c,p) lomo_player_seek(c,LOMO_FORMAT_PERCENT,p) // Br0ken

gint64 lomo_player_length(LomoPlayer *self, LomoFormat format);
#define lomo_player_length_time(c)    lomo_player_length(c,LOMO_FORMAT_TIME)
#define lomo_player_length_percent(c) lomo_player_length(c,LOMO_FORMAT_PERCENT) // Br0ken

// Volume and mute
gboolean lomo_player_set_volume(LomoPlayer *self, gint val);
gint lomo_player_get_volume(LomoPlayer *self);

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute);
gboolean lomo_player_get_mute(LomoPlayer *self);

gint    lomo_player_insert(LomoPlayer *self, LomoStream *stream, gint pos);
#define lomo_player_append(p,s)              lomo_player_insert(p,s,-1)
#define lomo_player_insert_uri(p,u,i)        lomo_player_insert(p,lomo_stream_new(i), i)
#define lomo_player_append_uri(p,u)          lomo_player_insert(p,lomo_stream_new(u),-1)

gint    lomo_player_insert_multi    (LomoPlayer *self, GList *streams, gint pos);
gint    lomo_player_insert_uri_strv (LomoPlayer *self, gchar **uris, gint pos);
gint    lomo_player_insert_uri_multi(LomoPlayer *self, GList *uris, gint pos);
#define lomo_player_append_multi(p,l)     lomo_player_insert_multi(p,l,-1)
#define lomo_player_append_uri_strv(p,v)  lomo_player_insert_uri_strv(p,v,-1)
#define lomo_player_append_uri_multi(p,l) lomo_player_insert_uri_multi(p,l,-1)

gboolean lomo_player_del(LomoPlayer *self, gint pos);

GList *lomo_player_get_playlist(LomoPlayer *self);

gint lomo_player_get_prev(LomoPlayer *self);
gint lomo_player_get_next(LomoPlayer *self);

LomoStream *lomo_player_get_nth(LomoPlayer *self, gint pos);
gint        lomo_player_get_position(LomoPlayer *self, LomoStream *stream);

gboolean          lomo_player_go_nth(LomoPlayer *self, gint pos, GError **error);
#define           lomo_player_go_prev(p,e) lomo_player_go_nth(p,lomo_player_get_prev(p),e)
#define           lomo_player_go_next(p,e) lomo_player_go_nth(p,lomo_player_get_next(p),e)

gint              lomo_player_get_current(LomoPlayer *self);
#define lomo_player_get_current_stream(p) lomo_player_get_nth(p, lomo_player_get_current(p))

guint lomo_player_get_total(LomoPlayer *self);

void lomo_player_clear(LomoPlayer *self);

void     lomo_player_set_repeat(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_repeat(LomoPlayer *self);
void     lomo_player_set_random(LomoPlayer *self, gboolean val);
gboolean lomo_player_get_random(LomoPlayer *self);

void lomo_player_randomize(LomoPlayer *self);

void lomo_player_print_pl(LomoPlayer *self);
void lomo_player_print_random_pl(LomoPlayer *self);


G_END_DECLS

#endif // __LOMO_PLAYER_H
