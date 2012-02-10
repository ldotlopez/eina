#include "gel-fs-scanner.h"
#include <string.h>

struct _GelFSScannerContext {
	GList *input_file_objects;
	GList *output_file_objects;
	GelFSScannerReadyFunc ready_func;
	GCompareFunc compare_func;
	GSourceFunc filter_func;
	gpointer user_data;
	GDestroyNotify notify;
};

static gboolean
_scheduler_job_helper(GIOSchedulerJob *job, GCancellable *cancellable, GelFSScannerContext *ctx);
static void
_scheduler_job_helper_finalize(GelFSScannerContext *ctx);
static void
_scanner_context_destroy(GelFSScannerContext *ctx);

/**
 * gel_fs_scanner_scan:
 * @file_objects: (element-type GFile) (transfer none): List of GFiles
 * @cancellable: (transfer full): A #GCancellable object
 * @ready_func: (scope call) (closure user_data): Function to run after scan is completed
 * @compare_func: (scope call) (allow-none): Function to compare and sort elements
 * @filter_func: (scope call) (allow-none): Filter function
 * @user_data: (closure) (allow-none): Custom data to pass to @ready_func
 * @notify: (scope notified) (allow-none): GDestroyNotify function for @user_data
 *
 * Scan a set of GFile objects recusivelly with an optional cancellable object.
 * After the scan is completed, the @ready_func is called to handle the results.
 * During scan @compare_func and @filter_func can be called multiple times in order to
 * sort and filter the elements found.
 *
 * Optionally the @ready_func can recieve extra data using the @user_data parameter. Whatever
 * the scan has been canceled or not @notify is called on @user_data after it has been
 * finished or canceled.
 */
void
gel_fs_scanner_scan(GList *file_objects, GCancellable *cancellable,
	GelFSScannerReadyFunc ready_func,
	GCompareFunc      compare_func,
	GSourceFunc       filter_func,
	gpointer user_data, GDestroyNotify notify)
{
	GelFSScannerContext *ctx = g_new0(GelFSScannerContext, 1);

	/* Store callbacks */
	ctx->ready_func = ready_func;
	ctx->compare_func   = compare_func;
	ctx->filter_func   = filter_func;

	/* Store input file objects */
	GList *iter = file_objects;
	while (iter != NULL)
	{
		GFile *f = G_FILE(iter->data);
		if (!f)
		{
			iter = iter->next;
			continue;
		}

		ctx->input_file_objects = g_list_prepend(ctx->input_file_objects, g_object_ref(f));
		iter = iter->next;
	}
	ctx->input_file_objects = g_list_reverse(ctx->input_file_objects);

	/* Store user_data and associated notify */
	ctx->user_data = user_data;
	ctx->notify    = notify;

	/* Run job in separate thread */
	g_io_scheduler_push_job ((GIOSchedulerJobFunc) _scheduler_job_helper,
		ctx, (GDestroyNotify) _scanner_context_destroy,
		0, cancellable);
}

/**
 * gel_fs_scanner_scan_uri_list:
 * @uri_list: (element-type utf8) (transfer none): List of URIs
 * @cancellable: (transfer full): A #GCancellable object
 * @ready_func: (scope call) (closure user_data): Function to run after scan is completed
 * @compare_func: (scope call) (allow-none): Function to compare and sort elements
 * @filter_func: (scope call) (allow-none): Filter function
 * @user_data: (closure) (allow-none): Custom data to pass to @ready_func
 * @notify: (scope notified) (allow-none): GDestroyNotify function for @user_data
 *
 * See gel_fs_scanner_scan()
 */
void gel_fs_scanner_scan_uri_list(GList *uri_list, GCancellable *cancellable,
    GelFSScannerReadyFunc ready_func,
    GCompareFunc compare_func,
    GSourceFunc  filter_func,
    gpointer user_data,
    GDestroyNotify notify)
{
	GList  *fs = NULL;
	for (GList *iter = uri_list; iter != NULL; iter = iter->next)
		fs = g_list_prepend(fs, g_file_new_for_uri((gchar *) iter->data));
	fs = g_list_reverse(fs);

	gel_fs_scanner_scan(fs, cancellable, ready_func, compare_func, filter_func, user_data, notify);
	g_list_foreach(fs, (GFunc) g_object_unref, NULL);
	g_list_free(fs);
}

/**
 * _scan_file_objects:
 * @input: (element-type GFile) (transfer none): List of #GFile objects to scan
 * @cancellable: (allow-none): a #GCancellable object
 *
 * Scans recursivelly a list of objects
 *
 * Returns: (element-type GFile) (transfer full): List of children (?)
 */
GList *
_scan_file_objects(GList *input, GCancellable *cancellable, GelFSScannerContext *ctx)
{
	GList *ret = NULL;

	/*
	 * Check for cancellation
	 */
	if (g_cancellable_is_cancelled(cancellable))
		return ret;

	/*
	 * Sanitize input list
	 */
	GList *tmp = NULL;
	for (GList *iter = input; iter != NULL; iter = iter->next)
	{
		/* Ensure GFile, this will leak any unknow objects but... how cares about them? */
		GFile *f = (GFile *) iter->data;
		if (!G_IS_FILE(f))
		{
			g_warn_if_fail(G_IS_FILE(f));
			continue;
		}

		/* Test for GFileInfo key or attach one */
		GFileInfo *info = g_object_get_data((GObject *) f, GEL_FS_SCANNER_INFO_KEY);
		if (!info)
		{
			info = g_file_query_info(f, "standard::*", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
			if (!info)
				g_warn_if_fail(G_IS_FILE_INFO(info));
			else
				g_object_set_data_full((GObject *) f, GEL_FS_SCANNER_INFO_KEY, info, g_object_unref);
		}

		/* Element is GFile and has a GFileInfo attached */
		tmp = g_list_prepend(tmp, iter->data);
	}
	input = g_list_reverse(tmp);

	/*
	 * Apply user filter
	 */
	if (ctx->filter_func)
	{
		GList *tmp = NULL;
		for (GList *iter = input; iter != NULL; iter = iter->next)
		{
			if (ctx->filter_func(iter->data))
				tmp = g_list_prepend(tmp, iter->data);
			else
				g_object_unref((GObject *) iter->data);
		}
		g_list_free(input);
		input = g_list_reverse(tmp);
	}

	/*
	 * Apply user sort
	 */
	if (ctx->compare_func)
		input = g_list_sort(input, ctx->compare_func);

	/*
	 * Recursive scan
	 */
	for (GList *iter = input; iter != NULL; iter = iter->next)
	{
		GFile *f        = (GFile     *) iter->data;
		GFileInfo *info = (GFileInfo *) g_object_get_data((GObject *) f, GEL_FS_SCANNER_INFO_KEY);
		if (!f || !info)
			continue;

		// Examine dirs
		if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
		{
			GFileEnumerator *enumerator = g_file_enumerate_children(f, "standard::*",
				G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);

			GList *children = NULL;
			while (TRUE)
			{
				GError *err = NULL;
				GFileInfo *child_info = g_file_enumerator_next_file(enumerator, NULL, &err);

				if (!child_info && (err == NULL))
					break;
				if (!child_info)
					continue;
				GFile *child = g_file_get_child(f, g_file_info_get_name(child_info));
				g_object_set_data_full((GObject *) child, GEL_FS_SCANNER_INFO_KEY, child_info, g_object_unref);

				children = g_list_prepend(children, child);
			}
			g_object_unref(enumerator);
			children = g_list_reverse(children);

			ret = g_list_concat(ret, _scan_file_objects(children, cancellable, ctx));
		}

		else if (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR)
			ret = g_list_append(ret, g_object_ref(f));
	}

	return ret;
}

/** 
 * _scheduler_job_helper:
 * @job: A #GIOSchedulerJob
 * @cancellable: A #GCancellable
 * @ctx: A #GelFSScannerContext
 *
 * Helper function meant to run in a separate thread, just calls the real function for filesystem scan and stores result into context.
 * Finally the finalize funcion is called in mainloop to notify original caller.
 *
 * Returns: FALSE
 */
static gboolean
_scheduler_job_helper(GIOSchedulerJob *job, GCancellable *cancellable, GelFSScannerContext *ctx)
{
	ctx->output_file_objects = _scan_file_objects(ctx->input_file_objects, cancellable, ctx);
	g_io_scheduler_job_send_to_mainloop(job, (GSourceFunc) _scheduler_job_helper_finalize, ctx, NULL);

	return FALSE;
}

/**
 * _scheduler_job_helper_finalize:
 * @ctx: A #GelFSScannerContext
 *
 * Helper function meant to run in the mainloop by the g_io_scheduler_push_job/g_io_scheduler_job_send_to_mainloop dance.
 */
static void
_scheduler_job_helper_finalize(GelFSScannerContext *ctx)
{
	ctx->ready_func(ctx->output_file_objects, ctx->user_data);
}

static void
_scanner_context_destroy(GelFSScannerContext *ctx)
{
	if (ctx->notify)
		ctx->notify(ctx->user_data);

	GList *all = g_list_concat(ctx->input_file_objects, ctx->output_file_objects);
	ctx->input_file_objects = ctx->output_file_objects = NULL;

	if (all)
	{
		g_list_foreach(all, (GFunc) g_object_unref, NULL);
		g_list_free(all);
	}

	g_free(ctx);
}

/**
 * gel_fs_scaner_compare_gfile_by_type_name:
 * @a: A #GFile objet
 * @b: A #GFile object
 *
 * Provides a compatible #GCompareFunc function to compare two #GFile based
 * on their type (folders first) at first and their name (simple strcmp) as
 * a second option
 *
 * Returns: 0, 1 or -1. See strcmp()
 */
gint
gel_fs_scaner_compare_gfile_by_type_name(GFile *a, GFile *b)
{
	gint r = gel_fs_scaner_compare_gfile_by_type(a, b);
	return (r == 0) ? gel_fs_scaner_compare_gfile_by_name(a,b) : r ;
}


/**
 * gel_fs_scaner_compare_gfile_by_type:
 * @a: A #GFile objet
 * @b: A #GFile object
 *
 * See gel_fs_scaner_compare_gfile_by_type_name()
 *
 * Returns: 0, 1 or -1. See strcmp()
 */
gint
gel_fs_scaner_compare_gfile_by_type (GFile *a, GFile *b)
{
	GFileInfo *aa = g_object_get_data((GObject *) a, GEL_FS_SCANNER_INFO_KEY);
	GFileInfo *bb = g_object_get_data((GObject *) b, GEL_FS_SCANNER_INFO_KEY);

	if (!aa || !bb)
		return gel_fs_scaner_compare_gfile_by_name(a, b);

	GFileType ta = g_file_info_get_file_type(aa);
	GFileType tb = g_file_info_get_file_type(bb);

	if ((ta == tb) && (ta == G_FILE_TYPE_DIRECTORY))
		return gel_fs_scaner_compare_gfile_by_name(a, b);
	if ((ta != tb) && (ta == G_FILE_TYPE_DIRECTORY))
		return -1;
	if ((ta != tb) && (tb == G_FILE_TYPE_DIRECTORY))
		return  1;

	return gel_fs_scaner_compare_gfile_by_name(a, b);
}

/**
 * gel_fs_scaner_compare_gfile_by_name:
 * @a: A #GFile objet
 * @b: A #GFile object
 *
 * See gel_fs_scaner_compare_gfile_by_type_name()
 *
 * Returns: 0, 1 or -1. See strcmp()
 */
gint
gel_fs_scaner_compare_gfile_by_name(GFile *a, GFile *b)
{
	GFileInfo *aa = g_object_get_data((GObject *) a, GEL_FS_SCANNER_INFO_KEY);
	GFileInfo *bb = g_object_get_data((GObject *) b, GEL_FS_SCANNER_INFO_KEY);

	if (aa && bb)
		return strcmp(g_file_info_get_name(aa), g_file_info_get_name(bb));
	else
	{
		g_warn_if_fail(aa && bb);
		gchar *a_uri = g_file_get_uri(a);
		gchar *b_uri = g_file_get_uri(b);

		gint r = strcmp(a_uri, b_uri);
		g_free(a_uri);
		g_free(b_uri);

		return r;
	}
}
