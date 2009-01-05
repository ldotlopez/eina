#ifndef __FS_H__
#define __FS_H__

#include <glib.h>
#include <gio/gio.h>
#include <lomo/player.h>

// Runs a file chooser dialog with all the stuff (cancel, loading, ...)
// and adds results to lomo handling play or enqueue
void
eina_fs_file_chooser_load_files(LomoPlayer *lomo);

// Checking input by uri or GFile
gboolean
eina_fs_is_supported_extension(gchar *uri);
gboolean
eina_fs_is_supported_file(GFile *uri);

// Sync get children for a (gchar*) uri using GVfs
GList*
eina_fs_uri_get_children(gchar *uri);

GSList*
eina_fs_files_from_uri_strv(gchar **uris);

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

