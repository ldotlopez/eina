/*
 * eina/fs.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::Fs"

#include <glib/gi18n.h>
#include <gel/gel.h>
#include <eina/eina-file-chooser-dialog.h>
#include <lomo/lomo-util.h>
#include "fs.h"

void
eina_fs_file_chooser_load_files(LomoPlayer *lomo)
{
	EinaFileChooserDialog *picker = eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	static gchar *uri = NULL;
	if (!uri)
	{
		const gchar *music_userdir = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
		if (!music_userdir)
			music_userdir = g_get_home_dir();
		uri = g_filename_to_uri(music_userdir, NULL, NULL);
	}
	gtk_file_chooser_set_current_folder_uri((GtkFileChooser *) picker, uri);

	gboolean run = TRUE;
	GSList *uris, *hold;

	do
	{
		run = FALSE;
		gint response = eina_file_chooser_dialog_run(picker);
		switch (response)
		{
		case EINA_FILE_CHOOSER_RESPONSE_PLAY:
			uris = eina_file_chooser_dialog_get_uris(picker);
			hold = gel_slist_filter(uris, (GelFilterFunc) eina_fs_is_supported_extension, NULL);
			hold = g_slist_sort(hold, (GCompareFunc) strcmp);
			g_slist_free(uris);

			if (hold == NULL) 
			{
				run = TRUE; // Keep alive
				break;
			}
			lomo_player_clear(lomo);
			lomo_player_append_uri_multi(lomo, (GList *) hold); g_slist_free(hold);
			lomo_player_play(lomo, NULL);
			run = FALSE; // Destroy
		break;

		case EINA_FILE_CHOOSER_RESPONSE_QUEUE:
			uris = eina_file_chooser_dialog_get_uris(picker);
			hold = gel_slist_filter(uris, (GelFilterFunc) eina_fs_is_supported_extension, NULL);
			hold = g_slist_sort(hold, (GCompareFunc) strcmp);
			g_slist_free(uris);

			if (hold == NULL)
			{
				run = TRUE; // Keep alive
				break;
			}	
			gchar *msg = g_strdup_printf(_("Loaded %d streams"), g_slist_length(hold));
			eina_file_chooser_dialog_set_msg(picker, EINA_FILE_CHOOSER_DIALOG_MSG_TYPE_INFO, msg);
			g_free(msg);

			lomo_player_append_uri_multi(lomo, (GList *) hold); g_slist_free(hold);

			run = TRUE; // Keep alive
			break;

		default:
			run = FALSE; // Destroy
			break;
		}
	} while (run);

	if (uri)
		g_free(uri);
	uri = gtk_file_chooser_get_current_folder_uri((GtkFileChooser *) picker);

	gtk_widget_destroy(GTK_WIDGET(picker));
}

gboolean
eina_fs_is_supported_extension(gchar *uri)
{
	static const gchar *suffixes[] = {".mp3", ".ogg", ".wma", ".aac", ".flac", ".m4a", NULL };
	gchar *lc_name;
	gint i;
	gboolean ret = FALSE;

	// lc_name = g_utf8_strdown(uri, -1);
	lc_name = g_ascii_strdown(uri, -1);
	for (i = 0; suffixes[i] != NULL; i++)
	{
		if (g_str_has_suffix(lc_name, suffixes[i]))
		{
			ret = TRUE;
			break;
		}
	}

	g_free(lc_name);
	return ret;
}

gboolean
eina_fs_is_supported_file(GFile *uri)
{
	gboolean ret;
	gchar *u = g_file_get_uri(uri);
	ret = eina_fs_is_supported_extension(u);
	g_free(u);
	return ret;
}

// Return a list with children's URIs from URI
// Returned list and list data must be free
GList*
eina_fs_uri_get_children(gchar *uri)
{
	GFile *f = NULL;
	GFileEnumerator *f_enum = NULL;
	GFileInfo *f_inf = NULL;
	GError *err = NULL;
	GList *ret = NULL;

	GFile *child = NULL;

	// Get an enumerator
	f = g_file_new_for_uri(uri);
	f_enum = g_file_enumerate_children(f, "standard::*", 0, NULL, &err);
	if (f == NULL) {
		gel_error(err->message);
		g_error_free(err);
		g_object_unref(f);
		return NULL;
	}

	// each child...
	while ((f_inf = g_file_enumerator_next_file(f_enum, NULL, &err)) != NULL)
	{
		if (f_inf == NULL) {
			gel_error(err->message);
			g_error_free(err);
			continue;
		}

		// Build URI, add to result and free temporal child's GFile
		// URI string is added to result with newly allocated memory
		child = g_file_get_child(f, g_file_info_get_name(f_inf));
		ret = g_list_prepend(ret, g_file_get_uri(child));
		g_object_unref(child);
	}

	// free resources: parent GFile, enumerator
	g_file_enumerator_close(f_enum, NULL, NULL);
	g_object_unref(f_enum);
	g_object_unref(f);

	return g_list_reverse(ret);
}

GSList*
eina_fs_files_from_uri_strv(gchar **uris)
{
	gint i;
	GSList *ret = NULL;
	gchar *uri;

	if (!uris)
		return NULL;
	
	for (i = 0; (uris[i] != NULL) && uris[i][0]; i++)
		if ((uri = lomo_create_uri(uris[i])) != NULL)
		{
			ret = g_slist_prepend(ret, g_file_new_for_uri(uri));
			g_free(uri);
		}
	return g_slist_reverse(ret);
}

GList *eina_fs_readdir(gchar *path, gboolean abspath) {
	/* Path is UTF8, g_dir_open require on-disk encoding (note: on windows is
	 * UTF8
	 */
	GDir *dir = NULL;
	const gchar* child;
	gchar *utf8_child;
	GList *ret = NULL, *ret2 = NULL;
	GError *err = NULL;

	gchar *real_path = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert UTF8 path '%s' to on-disk encoding: %s",
			path, err->message);
		goto epic_fail;
	}

	dir = g_dir_open(real_path, 0, &err);
	if (err != NULL) {
		gel_error("Cannot open dir '%s': '%s'", real_path, err->message);
		goto epic_fail;
	}

	while ((child = g_dir_read_name(dir)) != NULL) {
		utf8_child = g_filename_to_utf8(child, -1, NULL, NULL, &err);
		if (err != NULL) {
			gel_error("Cannot convert on-disk encoding '%s' to UTF8: %s",
				path, err->message);
			g_error_free(err);
			err = NULL;
			continue;
		}

		ret = g_list_prepend(ret, utf8_child);
	}
	g_dir_close(dir);

	if (abspath) {
		ret2 = eina_fs_prepend_dirname(path, ret);
		g_list_free(ret);
		ret = ret2;
	}

	return g_list_reverse(ret);

epic_fail:
	if (real_path != NULL)
		g_free(real_path);

	if (dir != NULL)
		g_dir_close(dir);

	if (err != NULL)
		g_error_free(err);

	return NULL;
}

GList *eina_fs_recursive_readdir(gchar *path, gboolean abspath) {
	GList *ret = NULL;
	GList *children, *l;

	l = children = eina_fs_readdir(path, abspath);
	while (l) {
		if (eina_fs_file_test((gchar*) l->data, G_FILE_TEST_IS_DIR)) 
			ret = g_list_concat(ret, eina_fs_recursive_readdir((gchar*) l->data, abspath));
		else
			ret = g_list_append(ret, (gchar*) l->data);
	}
	g_list_free(children);

	return ret;
}

GList*
eina_fs_prepend_dirname(gchar *dirname, GList *list)
{
	GList *ret = NULL;
	GList *l;

	l = list;
	while (l) {
		ret = g_list_append(ret, g_build_filename(dirname, l->data, NULL));
		l = l->next;
	}
	
	return g_list_reverse(ret);
}

gchar* eina_fs_utf8_to_ondisk(gchar *path)
{
	gchar *ret;
	GError *err = NULL;

	ret = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert UTF8 path '%s' to on-disk encoding: %s", path, err->message);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

gchar* eina_fs_ondisk_to_utf8(gchar *path)
{
	gchar *ret;
	GError *err = NULL;

	ret = g_filename_to_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL) {
		gel_error("Cannot convert on-disk encoding '%s' to UTF8 path: %s", path, err->message); g_error_free(err);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

gboolean eina_fs_file_test(gchar *utf8_path, GFileTest test)
{
	gboolean ret;
	gchar *real_path = eina_fs_utf8_to_ondisk(utf8_path);
	ret = g_file_test(real_path, test);
	g_free(real_path);
	return ret;
}

