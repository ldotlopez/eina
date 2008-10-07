#ifndef __LOMO_PLAYER_H__
#define __LOMO_PLAYER_H__

#include <glib-object.h>
#include <gst/gst.h>
#include "stream.h"

G_BEGIN_DECLS

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

#define LOMO_TYPE_PLAYER         (lomo_player_get_type ())
#define LOMO_PLAYER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), LOMO_TYPE_PLAYER, LomoPlayer))
#define LOMO_PLAYER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), LOMO_TYPE_PLAYER, LomoPlayerClass))
#define LOMO_IS_PLAYER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), LOMO_TYPE_PLAYER))
#define LOMO_IS_PLAYER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), LOMO_TYPE_PLAYER))
#define LOMO_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), LOMO_TYPE_PLAYER, LomoPlayerClass))

typedef struct {
	GstPipeline* (*create)  (GHashTable *opts);
	void         (*destroy) (GstPipeline *pipeline);

	gboolean     (*set_stream)  (GstPipeline *pipeline, const gchar *uri);
	gchar*       (*get_stream)  (GstPipeline *pipeline);

	GstStateChangeReturn (*set_state) (GstPipeline *pipeline, GstState state);
	GstState             (*get_state) (GstPipeline *pipeline);

	gboolean             (*query_position) (GstPipeline *pipeline, GstFormat *format, gint64 *position);
	gboolean             (*query_duration) (GstPipeline *pipeline, GstFormat *format, gint64 *duration);

	gboolean (*set_volume) (GstPipeline *pipeline, gint volume);
	gint     (*get_volume) (GstPipeline *pipeline);

	gboolean (*set_mute) (GstPipeline *pipeline, gboolean mute);
	gboolean (*get_mute) (GstPipeline *pipeline);
} LomoPlayerVTable;


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

GType		lomo_player_get_type   (void);

#define lomo_get_option_group() gst_init_get_option_group()

void lomo_init(gint *argc, gchar **argv[]);

// LomoPlayer       *lomo_player_new(gchar *audio_output, GError **error);
LomoPlayer       *lomo_player_new_with_opts(const gchar *option_name, ...);
#define lomo_player_new lomo_player_new_with_opts("audio-output", "autoaudiosink", NULL);

gboolean          lomo_player_reset(LomoPlayer *self, GError **error);
const LomoStream *lomo_player_get_stream(LomoPlayer *self);

// Quick play functions, simple shortcuts.
void        lomo_player_play_uri(LomoPlayer *self, gchar *uri, GError **error);
void        lomo_player_play_stream(LomoPlayer *self, LomoStream *stream, GError **error);

LomoStateChangeReturn lomo_player_set_state(LomoPlayer *self, LomoState state, GError **error);
#define lomo_player_play(p,error)  lomo_player_set_state(p,LOMO_STATE_PLAY, error)
#define lomo_player_pause(p,error) lomo_player_set_state(p,LOMO_STATE_PAUSE,error)
#define lomo_player_stop(p,error)  lomo_player_set_state(p,LOMO_STATE_STOP, error)
LomoState lomo_player_get_state(LomoPlayer *self);

gint64 lomo_player_tell(LomoPlayer *self, LomoFormat format);
#define lomo_player_tell_time(p)    lomo_player_tell(p,LOMO_FORMAT_TIME)
#define lomo_player_tell_percent(p) lomo_player_tell(p,LOMO_FORMAT_PERCENT)

gboolean lomo_player_seek(LomoPlayer *self, LomoFormat format, gint64 val);
#define lomo_player_seek_time(c,t)    lomo_player_seek(c,LOMO_FORMAT_TIME,t)
#define lomo_player_seek_percent(c,p) lomo_player_seek(c,LOMO_FORMAT_PERCENT,p) // Br0ken

gint64 lomo_player_length(LomoPlayer *self, LomoFormat format);
#define lomo_player_length_time(c)    lomo_player_length(c,LOMO_FORMAT_TIME)
#define lomo_player_length_percent(c) lomo_player_length(c,LOMO_FORMAT_PERCENT) // Br0ken

gboolean lomo_player_set_volume(LomoPlayer *self, gint val);
gint lomo_player_get_volume(LomoPlayer *self);

gboolean lomo_player_set_mute(LomoPlayer *self, gboolean mute);
gboolean lomo_player_get_mute(LomoPlayer *self);

gint    lomo_player_add_at_pos(LomoPlayer *self, LomoStream *stream, gint pos);
#define lomo_player_add(p,s)              lomo_player_add_at_pos(p,s,-1)
#define lomo_player_add_uri(p,u)          lomo_player_add_at_pos(p,lomo_stream_new(u),-1)
#define lomo_player_add_uri_at_pos(s,u,p) lomo_player_add_at_pos(s,lomo_stream_new(u), p)

gint lomo_player_add_uri_strv_at_pos(LomoPlayer *self, gchar **uris, gint pos);
gint lomo_player_add_uri_multi_at_pos(LomoPlayer *self, GList *uris, gint pos);
gint lomo_player_add_multi_at_pos(LomoPlayer *self, GList *streams, gint pos);

#define lomo_player_add_uri_multi(p,l) lomo_player_add_uri_multi_at_pos(p,l,-1)
#define lomo_player_add_uri_strv(p,v)  lomo_player_add_uri_strv_at_pos(p,v,-1)
#define lomo_player_add_multi(p,l)     lomo_player_add_multi_at_pos(p,l,-1)
gboolean lomo_player_del(LomoPlayer *self, gint pos);

const GList *lomo_player_get_playlist(LomoPlayer *self);

gint lomo_player_get_prev(LomoPlayer *self);
gint lomo_player_get_next(LomoPlayer *self);

const LomoStream *lomo_player_get_nth(LomoPlayer *self, gint pos);

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

#endif /* __LOMO_PLAYER_H__ */
