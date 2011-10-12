/*
 * eina/playlist/eina-playlist.h
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

#ifndef __EINA_PLAYLIST_H__
#define __EINA_PLAYLIST_H__

#include <glib-object.h>
#include <gel/gel-ui.h>
#include <lomo/lomo-player.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLAYLIST eina_playlist_get_type()

#define EINA_PLAYLIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLAYLIST, EinaPlaylist)) 
#define EINA_PLAYLIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_PLAYLIST, EinaPlaylistClass)) 
#define EINA_IS_PLAYLIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLAYLIST)) 
#define EINA_IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_PLAYLIST))
#define EINA_PLAYLIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_PLAYLIST, EinaPlaylistClass))

typedef struct _EinaPlaylistPrivate EinaPlaylistPrivate;
typedef struct {
	GelUIGeneric parent;
	EinaPlaylistPrivate *priv;
} EinaPlaylist;

typedef struct {
	GelUIGenericClass parent_class;
	gboolean (*action_activated) (EinaPlaylist *self, GtkAction *action);
} EinaPlaylistClass;

GType eina_playlist_get_type (void);

EinaPlaylist* eina_playlist_new (LomoPlayer *lomo);

LomoPlayer *eina_playlist_get_lomo_player(EinaPlaylist *self);

void   eina_playlist_set_stream_markup(EinaPlaylist *self, const gchar *markup);
gchar* eina_playlist_get_stream_markup(EinaPlaylist *self);

GtkTreeView  *eina_playlist_get_view (EinaPlaylist *self);
GtkTreeModel *eina_playlist_get_model(EinaPlaylist *self);

G_END_DECLS

#endif /* __EINA_PLAYLIST_H__ */
