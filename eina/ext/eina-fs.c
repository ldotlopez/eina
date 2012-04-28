/*
 * eina/ext/eina-fs.c
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

#define GEL_DOMAIN "Eina::Fs"

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "eina-fs.h"

#define EINA_FS_STATE_DOMAIN    EINA_APP_DOMAIN".states.file-chooser"
#define EINA_FS_LAST_FOLDER_KEY "last-folder"

#include <errno.h>
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gel/gel-io.h>
#include <lomo/lomo.h>
#include "eina-file-utils.h"
#include "eina-stock.h"

static void
load_from_uri_multiple_scanner_success_cb(GelIOScanner *scanner, GList *forest, EinaApplication *app);
static void
load_from_uri_multiple_scanner_error_cb(GelIOScanner *scanner, GFile *source, GError *error, EinaApplication *app);

GEL_DEFINE_QUARK_FUNC(eina_fs)

/**
 * eina_fs_load_gfile_array:
 * @app: An #EinaApplication
 * @files: (array length=n_files) (transfer none) (element-type GFile): GFiles to load
 * @n_files: Number of files
 *
 * Inserts recursively @files into @app
 */
void
eina_fs_load_gfile_array(EinaApplication *app, GFile **files, guint n_files)
{
	gchar **uri_strv = g_new0(gchar *, n_files + 1);
	for (guint i = 0; i < n_files; i++)
		uri_strv[i] = g_file_get_uri(files[i]);

	eina_fs_load_uri_strv(app, (const gchar *const *) uri_strv);
	g_strfreev(uri_strv);
}

/**
 * eina_fs_load_uri_strv:
 * @app: An #EinaApplication
 * @uris: (array zero-terminated=1) (transfer none) (element-type utf8): URIs to add
 */
void
eina_fs_load_uri_strv(EinaApplication *app,  const gchar *const *uris)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(uris != NULL);

	GList *uri_list = NULL;
	for (guint i = 0; uris[i]; i++)
		uri_list = g_list_prepend(uri_list, (gpointer) uris[i]);
	uri_list = g_list_reverse(uri_list);

	GelIOScanner *scanner = gel_io_scanner_new_full(uri_list, "standard::*", TRUE);
	g_signal_connect(scanner, "finish", (GCallback) load_from_uri_multiple_scanner_success_cb, app);
	g_signal_connect(scanner, "error",  (GCallback) load_from_uri_multiple_scanner_error_cb,   app);

	g_list_free(uri_list);
}

/**
 * eina_fs_load_playlist:
 * @app: An #EinaApplication
 * @playlist: Pathname to playlist file
 *
 * Loads playlist into @app
 */
void
eina_fs_load_playlist(EinaApplication *app, const gchar *playlist)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(g_file_test(playlist, G_FILE_TEST_IS_REGULAR));

	gchar *buffer = NULL;
	GError *error = NULL;
	gsize   reads = -1;
	if (!g_file_get_contents(playlist, &buffer, &reads, &error))
	{
		g_warning("%s", error->message);
		g_error_free(error);
		return;
	}
	if (reads == 0)
	{
		g_free(buffer);
		return;
	}

	gchar **lines = g_strsplit_set(buffer, "\n\r", 0);
	g_free(buffer);

	g_return_if_fail(lines && lines[0]);

	GFile **gfiles = g_new0(GFile*, g_strv_length(lines));

	guint n = 0;
	for (guint i = 0; lines[i]; i++)
		if (lines[i] && (lines[i][0] != '\0'))
			gfiles[n++] = g_file_new_for_commandline_arg(lines[i]);
	g_strfreev(lines);

	eina_fs_load_gfile_array(app, gfiles, n);
	for (guint i = 0; i < n; i++)
		g_object_unref(gfiles[i]);
	g_free(gfiles);
}

static void
load_from_uri_multiple_scanner_success_cb(GelIOScanner *scanner, GList *forest, EinaApplication *app)
{
	// Flatten is container transfer
	GList *flatten = gel_io_scanner_flatten_result(forest);

	gchar **uri_strv = g_new0(gchar *, g_list_length(flatten) + 1);
	guint  i = 0;
	GList *l = flatten;
	while (l)
	{
		GFile     *file = G_FILE(l->data);
		GFileInfo *info = g_object_get_data((GObject *) file, "g-file-info");
		gchar *uri = g_file_get_uri(file);

		if ((g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR) &&
			(eina_file_utils_is_supported_extension(uri)))
			uri_strv[i++] = uri;
		else
			g_free(uri);

		l = l->next;
	}

	LomoPlayer *lomo = eina_application_get_interface(app, "lomo");
	lomo_player_insert_strv(lomo,  (const gchar * const*) uri_strv, -1);
	g_strfreev(uri_strv);
}

static void
load_from_uri_multiple_scanner_error_cb(GelIOScanner *scanner, GFile *source, GError *error, EinaApplication *app)
{
	gchar *uri = g_file_get_uri(source);
	g_warning(_("'%s' throw an error: %s"), uri, error->message);
	g_free(uri);
}

/*
 * eina_fs_load_from_default_file_chooser:
 * @app: An #EinaApplication.
 *
 * Creates a new #EinaFileChooserDialog to select files and loads them into
 * @app. This functions uses eina_fs_load_from_uri_multiple() see doc.
 */
void
eina_fs_load_from_default_file_chooser(EinaApplication *app)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));

	static EinaFileChooserDialog *picker = NULL;

	if (picker)
	{
		gtk_window_present((GtkWindow *) picker);
		return;
	}

	picker = (EinaFileChooserDialog *) eina_file_chooser_dialog_new(
		(GtkWindow *) eina_application_get_window(app),
		EINA_FILE_CHOOSER_DIALOG_ACTION_LOAD_FILES);
	g_object_set((GObject *) picker,
		"title", _("Add or queue files"),
		NULL);
	GSettings *settings = eina_application_get_settings(app, EINA_FS_STATE_DOMAIN);

	const gchar *prev_folder_uri = g_settings_get_string(settings, EINA_FS_LAST_FOLDER_KEY);
	if ((prev_folder_uri != NULL) && (prev_folder_uri[0] != '\0'))
		gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(picker), prev_folder_uri);

	eina_fs_load_from_file_chooser(app, picker);

	gchar *curr_folder_uri = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(picker));
	g_settings_set_string(settings, EINA_FS_LAST_FOLDER_KEY, curr_folder_uri);
	g_free(curr_folder_uri);

	gtk_widget_destroy((GtkWidget *) picker);
	picker = NULL;
}

void
_eina_fs_dialog_subloop_response(EinaFileChooserDialog *dialog, gint response, EinaApplication *app)
{
	g_warn_if_fail(gtk_main_level() > 1);

	gint run = TRUE;

	if ((response != EINA_FILE_CHOOSER_RESPONSE_PLAY) && (response != EINA_FILE_CHOOSER_RESPONSE_QUEUE))
	{
		gtk_main_quit();
		return;
	}

	GList *uris = eina_file_chooser_dialog_get_uris(dialog);
	if (uris == NULL)
		return;

	LomoPlayer *lomo = eina_application_get_interface(app, "lomo");
	if (response == EINA_FILE_CHOOSER_RESPONSE_PLAY)
	{
		run = FALSE;
		lomo_player_clear(lomo);
	}

	gboolean do_play = (lomo_player_get_n_streams(lomo) == 0);
	gchar **tmp = gel_list_to_strv(uris, FALSE);
	lomo_player_insert_strv(lomo, (const gchar * const*) tmp, -1);
	g_free(tmp);
	gel_list_deep_free(uris, (GFunc) g_free);

	if (do_play)
		lomo_player_play(lomo, NULL);

	if (!run)
		gtk_main_quit();
}

static gboolean
_eina_fs_dialog_subloop_delete_event_cb(EinaFileChooserDialog *dialog, GdkEvent *ev, EinaApplication *app)
{
	gtk_dialog_response((GtkDialog *) dialog, GTK_RESPONSE_CLOSE);
	return TRUE;
}

/*
 * eina_fs_load_from_file_chooser:
 * @app: An #EinaApplication
 * @dialog: An #EinaFileChooserDialog
 *
 * Using and existing #EinaFileChooserDialog this function gets selected URIs
 * and loads them into @app. This functions uses
 * eina_fs_load_from_uri_multiple() see doc.
 */
void
eina_fs_load_from_file_chooser(EinaApplication *app, EinaFileChooserDialog *dialog)
{
	g_return_if_fail(EINA_IS_APPLICATION(app));
	g_return_if_fail(EINA_IS_FILE_CHOOSER_DIALOG(dialog));

	g_signal_connect((GObject *) dialog, "response", (GCallback) _eina_fs_dialog_subloop_response, app);
	g_signal_connect((GObject *) dialog, "delete-event", (GCallback) _eina_fs_dialog_subloop_delete_event_cb, app);
	gtk_widget_show((GtkWidget *) dialog);
	gtk_window_present((GtkWindow *) dialog);
	gtk_main();

	/*
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

		LomoPlayer *lomo = eina_application_get_interface(app, "lomo");
		if (response == EINA_FILE_CHOOSER_RESPONSE_PLAY)
		{
			run = FALSE;
			lomo_player_clear(lomo);
		}

		gboolean do_play = (lomo_player_get_n_streams(lomo) == 0);
		gchar **tmp = gel_list_to_strv(uris, FALSE);
		lomo_player_insert_strv(lomo, (const gchar * const*) tmp, -1);
		g_free(tmp);
		gel_list_deep_free(uris, (GFunc) g_free);

		if (do_play)
			lomo_player_play(lomo, NULL);
	}
	*/
}

/*
 * eina_fs_uri_get_children:
 * @uri: URI to read.
 *
 * Reads URI as a directory and returns his children in a URI form
 *
 * Returns: (transfer full) (element-type utf8): Children URIs.
 */
GList*
eina_fs_uri_get_children(const gchar *uri)
{
	g_return_val_if_fail(uri != NULL, NULL);

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
		g_warning("%s", err->message);
		g_error_free(err);
		g_object_unref(f);
		return NULL;
	}

	// each child...
	while ((f_inf = g_file_enumerator_next_file(f_enum, NULL, &err)) != NULL)
	{
		if (f_inf == NULL) {
			g_warning("%s", err->message);
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

/*
 * eina_fs_readdir:
 * @path: Path to read in utf-8. It will be converted to ondisk path.
 * @abspath: %TRUE if returned values should be absolute, %FALSE if not.
 *
 * Safe readdir with absolute paths option
 *
 * Returns: (transfer full) (element-type utf8): List of children.
 */
GList *eina_fs_readdir(const gchar *path, gboolean abspath) {
	g_return_val_if_fail(path != NULL, FALSE);

	/* Path is UTF8, g_dir_open require on-disk encoding (note: on windows is
	 * UTF8
	 */
	GDir *dir = NULL;
	const gchar* child;
	gchar *utf8_child;
	GList *ret = NULL, *ret2 = NULL;
	GError *err = NULL;

	gchar *real_path = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL)
	{
		g_warning(_("Cannot convert UTF8 path '%s' to on-disk encoding: %s"), path, err->message);
		goto eina_fs_readdir_fail;
	}

	dir = g_dir_open(real_path, 0, &err);
	if (err != NULL)
	{
		g_warning("Cannot open dir '%s': '%s'", real_path, err->message);
		goto eina_fs_readdir_fail;
	}

	while ((child = g_dir_read_name(dir)) != NULL)
	{
		utf8_child = g_filename_to_utf8(child, -1, NULL, NULL, &err);
		if (err)
		{
			g_warning(_("Cannot convert on-disk encoding '%s' to UTF8: %s"), path, err->message);
			g_error_free(err);
			err = NULL;
			continue;
		}

		ret = g_list_prepend(ret, utf8_child);
	}
	g_dir_close(dir);

	if (abspath)
	{
		ret2 = eina_fs_prepend_dirname(path, ret);
		g_list_free(ret);
		ret = ret2;
	}

	return g_list_reverse(ret);

eina_fs_readdir_fail:
	if (real_path != NULL)
		g_free(real_path);

	if (dir != NULL)
		g_dir_close(dir);

	if (err != NULL)
		g_error_free(err);

	return NULL;
}

/*
 * eina_fs_recursive_readdir:
 * @path: Path to recursively read
 * @abspath: Return absolute path or not
 *
 * Recursive read @path.
 *
 * Returns: (transfer full) (element-type utf8): A #GList of paths.
 */
GList *eina_fs_recursive_readdir(const gchar *path, gboolean abspath)
{
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

/*
 * eina_fs_prepend_dirname:
 * @dirname: foo?
 * @list: bar?
 *
 * Black magic
 *
 * Returns: (transfer full) (element-type utf8): Unicorns
 */
GList*
eina_fs_prepend_dirname(const gchar *dirname, GList *list)
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

/*
 * eina_fs_utf8_to_ondisk:
 * @path: Path in utf8 encoding
 *
 * Converts a path encoded in utf8 to path in disk encoding
 *
 * Returns: The path in disk encoding. This should be freeed with g_free
 */
gchar* eina_fs_utf8_to_ondisk(const gchar *path)
{
	g_return_val_if_fail(path != NULL, NULL);

	GError *err = NULL;

	gchar *ret = g_filename_from_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL)
	{
		g_warning("Cannot convert UTF8 path '%s' to on-disk encoding: %s", path, err->message);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

/*
 * eina_fs_ondisk_to_utf8:
 * @path: Path in disk encoding
 *
 * Converts a path encoded in disk charset to utf8
 *
 * Returns: The path in utf8 encoding. This should be freeed with g_free
 */
gchar* eina_fs_ondisk_to_utf8(const gchar *path)
{
	g_return_val_if_fail(path != NULL, NULL);

	GError *err = NULL;

	gchar *ret = g_filename_to_utf8(path, -1, NULL, NULL, &err);
	if (err != NULL)
	{
		g_warning("Cannot convert on-disk encoding '%s' to UTF8 path: %s", path, err->message); g_error_free(err);
		g_error_free(err);
		return NULL;
	}
	return ret;
}

/*
 * eina_fs_file_test:
 * @utf8_path: Path in utf8 encoding
 * @test: Test to perform
 *
 * Does @test on @utf8_path.
 *
 * Returns: %TRUE if test is successfull, %FALSE otherwise
 */
gboolean eina_fs_file_test(const gchar *utf8_path, GFileTest test)
{
	gboolean ret;
	gchar *real_path = eina_fs_utf8_to_ondisk(utf8_path);
	ret = g_file_test(real_path, test);
	g_free(real_path);
	return ret;
}

/*
 * eina_fs_mkdir:
 * @pathname: Path to create
 * @mode: Mode for the path
 * @error: (transfer full) (allow-none): return location for a GError, or %NULL.
 *
 * Creates a directory named @pathname with mode set to @mode. If operation
 * fails @error is filled with the error.
 *
 * Returns: TRUE on success, FALSE if an error occurred.
 */
gboolean
eina_fs_mkdir(const gchar *pathname, gint mode, GError **error)
{
	if (g_mkdir_with_parents(pathname, mode) == -1)
	{
		g_set_error_literal(error, eina_fs_quark(), errno, strerror(errno));
		return FALSE;
	}
	return TRUE;
}

/*
 * eina_fs_get_cache_dir:
 *
 * Get the cache dir for the current user
 *
 * Returns: The cache directory
 */
const gchar*
eina_fs_get_cache_dir(void)
{
	const gchar *ret = NULL;
	if (!ret)
		ret = g_build_filename(g_get_user_cache_dir(), gel_get_package_name(), NULL);
	return ret;
}

