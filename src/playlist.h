#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include <gel/gel.h>

typedef struct _EinaPlaylist EinaPlaylist;

gboolean playlist_init(GelHub *hub, gint *argc, gchar ***argv);
gboolean playlist_exit(gpointer data);

GtkWidget *playlist_get_widget(EinaPlaylist *self);

#endif

