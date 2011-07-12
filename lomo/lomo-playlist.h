#ifndef __LOMO_PLAYLIST_H__
#define __LOMO_PLAYLIST_H__

#include <glib-object.h>
#include <lomo/lomo-stream.h>

G_BEGIN_DECLS

#define LOMO_TYPE_PLAYLIST lomo_playlist_get_type()

#define LOMO_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOMO_TYPE_PLAYLIST, LomoPlaylist)) 
#define LOMO_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LOMO_TYPE_PLAYLIST, LomoPlaylistClass))
#define LOMO_IS_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOMO_TYPE_PLAYLIST))
#define LOMO_IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LOMO_TYPE_PLAYLIST))
#define LOMO_PLAYLIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), LOMO_TYPE_PLAYLIST, LomoPlaylistClass))

typedef struct _LomoPlaylistPrivate LomoPlaylistPrivate;
typedef struct {
	/* <private> */
	GObject parent;
	LomoPlaylistPrivate *priv;
} LomoPlaylist;

typedef struct {
	/* <private> */
	GObjectClass parent_class;
} LomoPlaylistClass;

typedef enum {
	LOMO_PLAYLIST_TRANSFORM_MODE_NORMAL_TO_RANDOM = 0x0,
	LOMO_PLAYLIST_TRANSFORM_MODE_RANDOM_TO_NORMAL = 0x1
} LomoPlaylistTransformMode;

GType lomo_playlist_get_type (void);

LomoPlaylist* lomo_playlist_new (void);

void     lomo_playlist_insert      (LomoPlaylist *self, LomoStream *stream, gint index);
void     lomo_playlist_insert_multi(LomoPlaylist *self, GList *streams, gint index);
void     lomo_playlist_remove      (LomoPlaylist *self, gint index);
gboolean lomo_playlist_swap        (LomoPlaylist *self, gint a, gint b);
void     lomo_playlist_clear       (LomoPlaylist *self);

const GList* lomo_playlist_get_playlist(LomoPlaylist *self);
const GList* lomo_playlist_get_random_playlist (LomoPlaylist *self);

LomoStream* lomo_playlist_get_nth_stream  (LomoPlaylist *self, gint index);
gint        lomo_playlist_get_stream_index(LomoPlaylist *self, LomoStream *stream);

gint     lomo_playlist_get_current(LomoPlaylist *self);
gboolean lomo_playlist_set_current(LomoPlaylist *self, gint index);
gint     lomo_playlist_get_n_streams(LomoPlaylist *self);

gboolean lomo_playlist_get_random(LomoPlaylist *self);
void     lomo_playlist_set_random(LomoPlaylist *self, gboolean val);
gboolean lomo_playlist_get_repeat(LomoPlaylist *self);
void     lomo_playlist_set_repeat(LomoPlaylist *self, gboolean val);

gint lomo_playlist_get_previous(LomoPlaylist *self);
gint lomo_playlist_get_next    (LomoPlaylist *self);

#define lomo_playlist_go_previous(self) lomo_playlist_set_current(self, lomo_playlist_get_previous(self))
#define lomo_playlist_go_next(self)     lomo_playlist_set_current(self, lomo_playlist_get_next(self))

gint lomo_playlist_transform_index(LomoPlaylist *self, gint index, LomoPlaylistTransformMode mode);

void lomo_playlist_print(LomoPlaylist *self);
void lomo_playlist_print_random(LomoPlaylist *self);

G_END_DECLS

#endif /* __LOMO_PLAYLIST_H__ */
