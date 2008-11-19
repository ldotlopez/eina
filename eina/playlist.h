#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include <eina/base.h>

#define EINA_PLAYLIST(p)             ((EinaPlaylist *) p)
#define GEL_HUB_GET_PLAYLIST(hub)    EINA_PLAYLIST(gel_hub_shared_get(hub,"playlist"))
#define EINA_BASE_GET_PLAYLIST(base) GEL_HUB_GET_PLAYLIST(EINA_BASE(base)->hub)

G_BEGIN_DECLS

typedef struct _EinaPlaylist EinaPlaylist;

G_END_DECLS

#endif // _PLAYLIST_H
