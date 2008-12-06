#ifndef __FS_H__
#define __FS_H__

#include <glib.h>
#include <gio/gio.h>
#include <lomo/player.h>

void
eina_fs_file_chooser_load_files(LomoPlayer *lomo);
/*
void
eina_fs_dnd_load_files(LomoPlayer *lomo);
*/
typedef enum {
	EINA_FS_FILTER_ACCEPT,
	EINA_FS_FILTER_REJECT
} EinaFsFilterAction;


typedef EinaFsFilterAction (*EinaFsFilterFunc)(GFileInfo *fileinfo);

void
eina_fs_lomo_feed_uri_multi(LomoPlayer *lomo, GList *uris, EinaFsFilterFunc filter, GCallback callback, gpointer data);

gboolean
eina_fs_is_supported_extension(gchar *uri);

void
eina_fs_readdir_async(gchar *uri, gpointer data);

#if 0
GList *
eina_fs_parse_playlist_buffer(gchar *buffer);
#endif

GList*
eina_fs_uri_get_children(gchar *uri);

/*
 * Utility functions to handle on-disk files, paths, dirs, etc...
 */
GList* eina_fs_readdir(gchar *path, gboolean abspath);
GList *eina_fs_recursive_readdir(gchar *path, gboolean abspath);
GList* eina_fs_prepend_dirname(gchar *dirname, GList *list);

gchar* eina_fs_utf8_to_ondisk(gchar *path);
gchar* eina_fs_ondisk_to_utf8(gchar *path); 

gboolean eina_fs_file_test(gchar *utf8_path, GFileTest test);

#endif

