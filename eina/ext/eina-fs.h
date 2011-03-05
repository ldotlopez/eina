/*
 * eina/ext/eina-fs.h
 *
 * Copyright (C) 2004-2010 Eina
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

#ifndef __FS_H__
#define __FS_H__

#include <glib.h>
#include <gel/gel.h>
#include <eina/ext/eina-application.h>
#include <eina/ext/eina-file-chooser-dialog.h>

void eina_fs_load_files_multiple(EinaApplication *app, GFile **files, guint n_files);

void eina_fs_load_from_uri_multiple        (EinaApplication *app, GList *uris);
void eina_fs_load_from_default_file_chooser(EinaApplication *app);
void eina_fs_load_from_file_chooser        (EinaApplication *app, EinaFileChooserDialog *dialog);

GSList* eina_fs_files_from_uri_strv(const gchar *const *uris);
GList*  eina_fs_uri_get_children(const gchar *uri);
GList*  eina_fs_readdir(const gchar *path, gboolean abspath);

GList* eina_fs_recursive_readdir(const gchar *path, gboolean abspath);
GList* eina_fs_prepend_dirname(const gchar *dirname, GList *list);

gchar* eina_fs_utf8_to_ondisk(const gchar *path);
gchar* eina_fs_ondisk_to_utf8(const gchar *path); 

gboolean eina_fs_file_test(const gchar *utf8_path, GFileTest test);
gboolean eina_fs_mkdir    (const gchar *pathname, gint mode, GError **error);

const gchar* eina_fs_get_cache_dir(void);

#endif

