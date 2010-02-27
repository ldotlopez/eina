/*
 * eina/fs.c
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

#define GEL_DOMAIN "Eina::Fs"

#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gel/gel-io.h>
#include <lomo/lomo-util.h>
#include <eina/ext/eina-file-chooser-dialog.h>
#include <eina/ext/eina-file-utils.h>
#include <eina/ext/eina-stock.h>
#include <eina/fs.h>
#include <eina/lomo.h>
#include <eina/settings.h>

static void
load_from_uri_multiple_scanner_success_cb(GelIOScanner *scanner, GList *forest, GelApp *app);
static void
load_from_uri_multiple_scanner_error_cb(GelIOScanner *scanner, GFile *source, GError *error, GelApp *app);

/*
 * eina_fs_load_from_uri_multiple:
 * @lomo: a #LomoPlayer
 * @uris: list of uris (gchar *) to load, this function deep copies it
 *
 * Scans and add uris to lomo
 */
void
eina_fs_load_from_uri_multiple(GelApp *app, GList *uris)
{
	GelIOScanner *scanner = gel_io_scanner_new_full(uris, "standard::*", TRUE);
	g_signal_connect(scanner, "finish", (GCallback) load_from_uri_multiple_scanner_success_cb, app);
	g_signal_connect(scanner, "error",  (GCallback) load_from_uri_multiple_scanner_error_cb,   app);
	/*
	gel_io_scan(uris, "standard::*",  TRUE,
		(GelIOScannerSuccessFunc) load_from_uri_multiple_scanner_success_cb, 
		(GelIOScannerErrorFunc)   load_from_uri_multiple_scanner_error_cb,
		app);
		*/
}

static void
load_from_uri_multiple_scanner_success_cb(GelIOScanner *scanner, GList *forest, GelApp *app)
{
	// GList *flatten = gel_io_scan_flatten_result(forest);
	GList *flatten = gel_io_scanner_flatten_result(forest);
	GList *l = flatten;
	GList *uris = NULL;
	while (l)
	{
		GFile     *file = G_FILE(l->data);
		GFileInfo *info = g_object_get_data((GObject *) file, "g-file-info");
		gchar *uri = g_file_get_uri(file);
	
		if ((g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR) &&
			(eina_file_utils_is_supported_extension(uri)))
			uris = g_list_prepend(uris, uri);
		else
			g_free(uri);

		l = l->next;
	}
	uris = g_list_reverse(uris);

	LomoPlayer *lomo = gel_app_get_lomo(app);
	gboolean do_play = ((lomo_player_get_total(lomo) == 0) && eina_conf_get_boolean(gel_app_get_settings(app), "/core/auto-play", TRUE));
	lomo_player_append_uri_multi(lomo, uris);
	if (do_play)
		lomo_player_play(lomo, NULL);

	gel_list_deep_free(uris, (GFunc) g_free);
	g_list_free(flatten);
}

static void
load_from_uri_multiple_scanner_error_cb(GelIOScanner *scanner, GFile *source, GError *error, GelApp *app)
{
	gchar *uri = g_file_get_uri(source);
	gel_warn(N_("'%s' throw an error: %s"), error->message);
	g_free(uri);
}


void
eina_fs_load_from_default_file_chooser(GelApp *app)
{
	EinaFileChooserDialog *picker = (EinaFileChooserDialog *) eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	g_object_set((GObject *) picker,
		"title", N_("Add or queue files"),
		NULL);
	eina_fs_load_from_file_chooser(app, picker);
	gtk_widget_destroy((GtkWidget *) picker);
}

void
eina_fs_load_from_file_chooser(GelApp *app, EinaFileChooserDialog *dialog)
{
	gboolean run = TRUE;
	while (run)
	{
		gint response = gtk_dialog_run((GtkDialog *) dialog);
		if ((response != EINA_FILE_CHOOSER_RESPONSE_PLAY) && (response != EINA_FILE_CHOOSER_RESPONSE_QUEUE))
		{
			run = FALSE;
			break;
		}

		GList *uris = eina_file_chooser_dialog_get_uris(dialog);
		if (uris == NULL)
			continue;

		LomoPlayer *lomo = gel_app_get_lomo(app);
		if (response == EINA_FILE_CHOOSER_RESPONSE_PLAY)
		{
			run = FALSE;
			lomo_player_clear(lomo);
		}

		gboolean do_play = (lomo_player_get_total(lomo) == 0);
		lomo_player_append_uri_multi(lomo, uris);
		gel_list_deep_free(uris, (GFunc) g_free);

		if (do_play)
			lomo_player_play(lomo, NULL);
	}
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

