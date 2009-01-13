#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_PLAYLIST(p)           ((EinaPlaylist *) p)
#define GEL_APP_GET_PLAYLIST(app)  EINA_PLAYLIST(gel_app_shared_get(app,"playlist"))
#define EINA_OBJ_GET_PLAYLIST(obj) GEL_HUB_GET_PLAYLIST(eina_obj_get_app(obj))

typedef struct _EinaPlaylist EinaPlaylist;

G_END_DECLS

#endif // _PLAYLIST_H
