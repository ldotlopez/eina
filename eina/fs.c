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
#include <eina/ext/eina-file-chooser-dialog.h>
#include <lomo/lomo-util.h>
#include <eina/fs.h>
#include <gel/gel-io.h>
#include <eina/ext/eina-stock.h>

typedef struct
{
	LomoPlayer  *lomo;
	GQueue      *inputs;
	GList       *results;
	GelIOTreeOp *op;
	GelJobQueue *jq;
} FsLoadUriMultipleJob;

static FsLoadUriMultipleJob*
fs_load_uri_multiple_job_new(LomoPlayer *lomo)
{
	FsLoadUriMultipleJob *self = g_new0(FsLoadUriMultipleJob, 1);
	self->lomo   = lomo;
	self->jq     = gel_job_queue_new();
	self->inputs = g_queue_new();
	return self;
}

static void
fs_load_uri_multiple_job_free(FsLoadUriMultipleJob *self)
{
	if (self->inputs)
	{
		g_queue_foreach(self->inputs, (GFunc) g_free, NULL);
		g_queue_free(self->inputs);
	}

	if (self->results)
	{
		gel_list_deep_free(self->results, (GFunc) g_free);
		self->results = NULL;
	}

	gel_free_and_invalidate(self->op, NULL, gel_io_tree_op_close);
	gel_free_and_invalidate(self->jq, NULL, g_object_unref);
	g_free(self);
}

static void
fs_load_from_uri_multiple_tree_success_cb(GelIOTreeOp *op, const GFile *root, const GNode *results, FsLoadUriMultipleJob *job_data);
static void
fs_load_from_uri_multiple_tree_error_cb(GelIOTreeOp *op, const GFile *file, const GError *error, FsLoadUriMultipleJob *job_data);
static void
fs_load_from_uri_multiple_job_run_cb(GelJobQueue *jq, FsLoadUriMultipleJob *job_data);
static void
fs_load_from_uri_multiple_job_cancel_cb(GelJobQueue *jq, FsLoadUriMultipleJob *job_data);

/*
 * eina_fs_load_from_uri_multiple:
 * @lomo: a #LomoPlayer
 * @uris: list of uris (gchar *) to load, this function deep copies it
 *
 * Scans and add uris to lomo
 */
void
eina_fs_load_from_uri_multiple(LomoPlayer *lomo, GList *uris)
{
	FsLoadUriMultipleJob *job_data = fs_load_uri_multiple_job_new(lomo);

	GList *l = uris;
	while (l)
	{
		g_queue_push_tail(job_data->inputs, g_strdup((gchar *) l->data));
		l = l->next;
	}

	gel_job_queue_push_job(job_data->jq,
		(GelJobCallback) fs_load_from_uri_multiple_job_run_cb,
		(GelJobCallback) fs_load_from_uri_multiple_job_cancel_cb,
		job_data);
	gel_job_queue_run(job_data->jq);
}

static void
fs_load_from_uri_multiple_job_run_cb(GelJobQueue *jq, FsLoadUriMultipleJob *job_data)
{
	if (g_queue_is_empty(job_data->inputs))
	{
		fs_load_uri_multiple_job_free(job_data);
		return;
	}

	gchar *uri = g_queue_pop_head(job_data->inputs);
	GFile *file = g_file_new_for_uri(uri);
	g_free(uri);

	job_data->op = gel_io_tree_walk(file,
		"standard::*",
		TRUE,
		(GelIOTreeSuccessFunc) fs_load_from_uri_multiple_tree_success_cb,
		(GelIOTreeErrorFunc) fs_load_from_uri_multiple_tree_error_cb,
		job_data
		);
}

static void
fs_load_from_uri_multiple_job_cancel_cb(GelJobQueue *jq, FsLoadUriMultipleJob *job_data)
{
	fs_load_uri_multiple_job_free(job_data);
}

static void
fs_load_from_uri_multiple_tree_success_cb(GelIOTreeOp *op, const GFile *root, const GNode *results, FsLoadUriMultipleJob *job_data)
{
	// Extract uris for regular files and links
	GList *valid_type = NULL;
	GList *l, *flat = gel_io_tree_result_flatten(results);
	for (l = flat; l != NULL; l = l->next)
	{
		GFile *file = G_FILE(l->data);
		GFileInfo *info = gel_file_get_file_info(file);
		GFileType  type = g_file_info_get_file_type(info);

		if ((type == G_FILE_TYPE_REGULAR) || (type == G_FILE_TYPE_SYMBOLIC_LINK))
			valid_type = g_list_prepend(valid_type, g_file_get_uri(file));
	}
	g_list_free(flat);

	// Filter supported extensions
	GList *accepted = NULL, *rejected = NULL;
	gel_list_bisect(valid_type, &accepted, &rejected, (GelFilterFunc) eina_fs_is_supported_extension, NULL);

	g_list_foreach(rejected, (GFunc) g_free, NULL);
	g_list_free(rejected);

	// Add results
	job_data->results = g_list_concat(job_data->results, accepted);
	gchar *root_ = g_file_get_uri((GFile *) root);
	gel_warn("Got %d results in %s", g_list_length(accepted), root_);
	g_free(root_);

	// Fire next job
	if (!g_queue_is_empty(job_data->inputs))
	{
		gel_job_queue_push_job(job_data->jq,
			(GelJobCallback) fs_load_from_uri_multiple_job_run_cb,
			(GelJobCallback) fs_load_from_uri_multiple_job_cancel_cb,
			job_data);
		gel_job_queue_next(job_data->jq);
	}
	else
	{
		lomo_player_append_uri_multi(job_data->lomo, job_data->results);
		fs_load_uri_multiple_job_free(job_data);
	}
}

static void
fs_load_from_uri_multiple_tree_error_cb(GelIOTreeOp *op, const GFile *file, const GError *error, FsLoadUriMultipleJob *job_data)
{
	gchar *uri = g_file_get_uri((GFile *) file);
	gel_warn(N_("Error with '%s': %s"), uri, error->message);
	g_free(uri);
}

void
eina_fs_file_chooser_load_files(LomoPlayer *lomo)
{
	EinaFileChooserDialog *picker = (EinaFileChooserDialog *) eina_file_chooser_dialog_new(EINA_FILE_CHOOSER_DIALOG_LOAD_FILES);
	g_object_set((GObject *) picker,
		"title", N_("Add or queue files"),
		NULL);

	gboolean run = TRUE;
	while (run)
	{
		gint response = gtk_dialog_run((GtkDialog *) picker);
		if ((response != EINA_FILE_CHOOSER_RESPONSE_PLAY) && (response != EINA_FILE_CHOOSER_RESPONSE_QUEUE))
		{
			run = FALSE;
			break;
		}

		GSList *uris = eina_file_chooser_dialog_get_uris(picker);
		if (uris == NULL)
			continue;

		GSList *supported = gel_slist_filter(uris, (GelFilterFunc) eina_fs_is_supported_extension, NULL);
		if (!supported)
		{
			g_slist_foreach(uris, (GFunc) g_free, NULL);
			g_slist_free(uris);
			continue;
		}

		if (response == EINA_FILE_CHOOSER_RESPONSE_PLAY)
		{
			run = FALSE;
			lomo_player_clear(lomo);
		}

		gboolean do_play = (lomo_player_get_total(lomo) == 0);
		lomo_player_append_uri_multi(lomo, (GList *) supported);
		g_slist_foreach(uris, (GFunc) g_free, NULL);
		g_slist_free(uris);
		g_slist_free(supported);
		if (do_play)
			lomo_player_play(lomo, NULL);
	}
	gtk_widget_destroy((GtkWidget *) picker);
}

gboolean
eina_fs_is_supported_extension(gchar *uri)
{
	static const gchar *suffixes[] = {".mp3", ".ogg", "wav", ".wma", ".aac", ".flac", ".m4a", NULL };
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

