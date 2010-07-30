/* eina-playlist.h */

#ifndef _EINA_PLAYLIST
#define _EINA_PLAYLIST

#include <glib-object.h>
#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLAYLIST eina_playlist_get_type()

#define EINA_PLAYLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLAYLIST, EinaPlaylist))

#define EINA_PLAYLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PLAYLIST, EinaPlaylistClass))

#define EINA_IS_PLAYLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLAYLIST))

#define EINA_IS_PLAYLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PLAYLIST))

#define EINA_PLAYLIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PLAYLIST, EinaPlaylistClass))

typedef struct {
	GelUIGeneric parent;
} EinaPlaylist;

typedef struct {
	GelUIGenericClass parent_class;
} EinaPlaylistClass;

GType eina_playlist_get_type (void);

EinaPlaylist* eina_playlist_new (void);

void
eina_playlist_set_lomo_player(EinaPlaylist *playlist, LomoPlayer *lomo);
void
eina_playlist_set_stream_markup(EinaPlaylist *playlist, gchar *markup);
gchar*
eina_playlist_get_stream_markup(EinaPlaylist *playlist);

G_END_DECLS

#endif /* _EINA_PLAYLIST */
