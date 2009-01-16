/*
 * gel/gel-io-ops.c
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

#define GEL_DOMAIN "Gel::IO::SimpleOps"
#include <string.h>
#include <gio/gio.h>
#include <gel/gel.h>
#include <gel/gel-io-misc.h>
#include <gel/gel-io-ops.h>
#include <gel/gel-io-op-result.h>

typedef gboolean (*GelIOOpHook) (GelIOOp *self);

struct _GelIOOp {
	guint refs;
	GFile *source;
	GCancellable *cancellable;
	GelIOOpSuccessFunc success;
	GelIOOpErrorFunc   error;
	GelIOOpHook cancel, destroy;
	gpointer _d, data;
};


static void
cancellable_cancelled_cb(GCancellable *cancellable, gpointer data);

GelIOOp *
gel_io_op_new(GFile *source, GCancellable *cancellable,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	GelIOOpHook cancel, GelIOOpHook destroy,
	gpointer data)
{
	GelIOOp *self = g_new0(GelIOOp, 1);

	self->source = source;
	self->cancellable = cancellable;
	self->success = success;
	self->error   = error;
	self->cancel  = cancel;
	self->destroy = destroy;

	self->data = data;

	if (self->cancellable)
		g_signal_connect(self->cancellable, "cancelled", (GCallback) cancellable_cancelled_cb, self);

	return self;
}

void
gel_io_op_destroy(GelIOOp *self)
{
	gel_io_op_cancel(self);

	if (self->destroy && self->destroy(self))
		return;

	if (self->cancellable)
	{
		g_signal_handlers_disconnect_by_func(self->cancellable, cancellable_cancelled_cb, self);
		if (!g_cancellable_is_cancelled(self->cancellable))
			g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
	}

	if (self->source)
		gel_free_and_invalidate(self->source, NULL, g_object_unref);

	g_free(self);
}

void
gel_io_op_ref(GelIOOp *self)
{
	self->refs++;
}

void
gel_io_op_unref(GelIOOp *self)
{
	self->refs--;
	if (self->refs < 1)
		gel_io_op_destroy(self);
}

// User cancels operation explicitly
void
gel_io_op_cancel(GelIOOp *self)
{
	if (self->cancel && self->cancel(self))
			return;

	if (self->cancellable)
	{
		g_signal_handlers_disconnect_by_func(self->cancellable, cancellable_cancelled_cb, self);
		if (!g_cancellable_is_cancelled(self->cancellable))
			g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
	}
}

// Call user's success callback
static void
gel_io_op_success(GelIOOp *self, GelIOOpResult *result)
{
	if (self->success)
		self->success(self, self->source, result, self->data);

}

// Call user's error callback
void
gel_io_op_error(GelIOOp *self, GError *error)
{
	if (self->error)
		self->error(self, self->source, error, self->data);
}

// Handle 'cancelled' signal to call user's error callback
static void
cancellable_cancelled_cb(GCancellable *cancellable, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);
	GError  *err = NULL;

	if (!g_cancellable_set_error_if_cancelled(cancellable, &err))
		gel_warn("Calling in wrong state");

	if (self->error)
		self->error(self, self->source, err, self->data);

	if (err)
		g_error_free(err);
}

// --
// file_read operation (DEPRECATED, glib-io has equivalent function)
// --
#ifndef GEL_IO_DISABLE_DEPRECATED
#define FILE_READ(p) ((GelIOOpFileRead*)p)
typedef struct {
	guint8 *buffer;
	GByteArray *ba;
} GelIOOpFileRead;

static void
read_file_open_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_file_read_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_file_close_cb(GObject *source, GAsyncResult *res, gpointer data);

gboolean
read_file_destroy(GelIOOp *self)
{
	GelIOOpFileRead *d = FILE_READ(self->_d);
	gel_free_and_invalidate(d->buffer, NULL, g_free);
	gel_free_and_invalidate_with_args(d->ba, NULL, g_byte_array_free, TRUE);
	g_free(d);

	self->_d = NULL;

	return FALSE; // Run 'normal' destroy too
}

GelIOOp *gel_io_read_file(GFile *file,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(file, g_cancellable_new(),
		success, error,
		NULL, read_file_destroy,
		data);
	self->_d = g_new0(GelIOOpFileRead, 1);
	g_file_read_async(file, G_PRIORITY_DEFAULT,
		self->cancellable, read_file_open_cb,
		self);
	return self;
}

static void
read_file_open_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(g_file_read_finish(G_FILE(source), res, &err));

	if (err != NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	FILE_READ(self->_d)->buffer = g_new0(guint8, 32*1024);  // 32kb
	g_input_stream_read_async(stream, FILE_READ(self->_d)->buffer, 32*1024,
		G_PRIORITY_DEFAULT, self->cancellable,
	read_file_read_cb, self);
}

static void
read_file_read_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError *err = NULL;
	gssize        readed;
	GInputStream *stream = G_INPUT_STREAM(source);

	readed = g_input_stream_read_finish(stream, res, &err);
	if (err != NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	if (readed > 0)
	{
		GelIOOpFileRead *fr = FILE_READ(self->_d);
		if (fr->ba == NULL)
			fr->ba = g_byte_array_new();

		g_byte_array_append(fr->ba, fr->buffer, readed);
		g_free(fr->buffer);
		g_input_stream_read_async(stream, fr->buffer, 32*1024,
			G_PRIORITY_DEFAULT, self->cancellable, read_file_read_cb, self);
		return;
	}

	g_free(FILE_READ(self->_d)->buffer);
	g_input_stream_close_async(stream, G_PRIORITY_DEFAULT, self->cancellable,
		read_file_close_cb, self);
}

static void
read_file_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError       *err = NULL;
	GInputStream *stream = G_INPUT_STREAM(source);

	if (!g_input_stream_close_finish(stream, res, &err))
	{
		gel_io_op_error(self, err);
		return;
	}

	GelIOOpResult *result = gel_io_op_result_new(GEL_IO_OP_RESULT_BYTE_ARRAY, FILE_READ(self->_d)->ba);
	gel_io_op_success(self, result);
	g_free(result);
}
#endif

// --
// read_dir operation
// --
static void
read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data);

gboolean
read_dir_destroy(GelIOOp *self)
{
	gel_list_deep_free((GList *) self->_d, g_object_unref);
	self->_d = NULL;
	return FALSE; // Continue destroying
}

GelIOOp *gel_io_read_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(dir, g_cancellable_new(),
		success, error,
		NULL, read_dir_destroy,
		data);
    g_file_enumerate_children_async(dir,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		read_dir_enumerate_cb,
		self);
	return self;
}

static void
read_dir_enumerate_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GFile           *dir;
	GFileEnumerator *enumerator;
	GError          *err = NULL;

	dir = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(dir, res, &err);
	if (enumerator == NULL)
	{
		gel_io_op_error(self, err);
		return;
	}

	g_file_enumerator_next_files_async(enumerator,
		4, G_PRIORITY_DEFAULT,
		self->cancellable,
		read_dir_next_files_cb,
		self);
}

static void
read_dir_next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);
	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	GList           *items = g_file_enumerator_next_files_finish(enumerator, res, &err);

	if (err != NULL)
	{
		gel_list_deep_free(items, g_object_unref);
		gel_io_op_error(self, err);
		return;
	}

	if (g_list_length(items) > 0)
	{
		self->_d = (gpointer) g_list_concat((GList *) self->_d, items);

		g_file_enumerator_next_files_async(enumerator,
			4, G_PRIORITY_DEFAULT,
			self->cancellable,
			read_dir_next_files_cb,
			data);
	}
	else
	{
		g_file_enumerator_close_async(enumerator,
			G_PRIORITY_DEFAULT,
			self->cancellable,
			read_dir_close_cb,
			data);
	}
}

static void
read_dir_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GError          *err = NULL;
	GFileEnumerator *enumerator = G_FILE_ENUMERATOR(source);
	gboolean         result = g_file_enumerator_close_finish(enumerator, res, &err);

	if (result == FALSE)
	{
		gel_io_op_error(self, err);
	}
	else
	{
		GelIOOpResult *r = gel_io_op_result_new(GEL_IO_OP_RESULT_OBJECT_LIST, self->_d);
		gel_io_op_success(self, r);
		g_free(r);
	}
}

// --
// recurse dir operation
// --

typedef struct {
	const gchar        *attributes;
	GelIOOpSuccessFunc  user_success;
	GelIOOpErrorFunc    user_error;
	GQueue             *queue;
	GHashTable         *blacklist;
	GelIOOp            *current;
	GelIORecurseTree   *tree;
} GelIOOpRecurseDir;
#define RECURSE_DIR(p) ((GelIOOpRecurseDir*)p)

static void
recurse_dir_success_cb(GelIOOp *op, GFile *parent, GelIOOpResult *result, gpointer data);
static void
recurse_dir_error_cb(GelIOOp *op, GFile *parent, GError *error, gpointer data);
static void
recurse_dir_run_queue(GelIOOp *self);

gboolean
recurse_dir_cancel(GelIOOp *self)
{
	GelIOOpRecurseDir *d = RECURSE_DIR(self->_d);
	if (!d)
		return TRUE;

	if (d->current)
	{
		gel_io_op_cancel(d->current);
		gel_io_op_unref(d->current);
		d->current = NULL;
	}

	if (!g_queue_is_empty(d->queue))
	{
		g_queue_foreach(d->queue, (GFunc) g_object_unref, NULL);
		g_queue_clear(d->queue);
	}

	if (d->blacklist)
		g_hash_table_remove_all(d->blacklist);

	return TRUE; // I do all the work
}

// Destroy a recurse dir struct
gboolean
recurse_dir_destroy(GelIOOp *self)
{
	GelIOOpRecurseDir *d = RECURSE_DIR(self->_d);

	if (!d)
		return FALSE;

	gel_free_and_invalidate(d->queue, NULL, g_queue_free);
	gel_free_and_invalidate(d->blacklist, NULL, g_hash_table_destroy);
	g_free(self->_d);

	return FALSE; // continue destroy
}

GelIOOp*
gel_io_recurse_dir(GFile *dir, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(dir, NULL,
		success, error, 
		recurse_dir_cancel, recurse_dir_destroy,
		data);
	self->_d = g_new0(GelIOOpRecurseDir, 1);
	RECURSE_DIR(self->_d)->tree = gel_io_recurse_tree_new();
	RECURSE_DIR(self->_d)->attributes = attributes;
	RECURSE_DIR(self->_d)->queue = g_queue_new();
	RECURSE_DIR(self->_d)->blacklist = g_hash_table_new(g_str_hash, g_str_equal);

	g_queue_push_tail(RECURSE_DIR(self->_d)->queue, dir);
	recurse_dir_run_queue(self);

	return self;
}

static void
recurse_dir_run_queue(GelIOOp *self)
{
	GelIOOpRecurseDir *d = RECURSE_DIR(self->_d);
	if (d == NULL)
	{
		return;
	}

	if (g_queue_is_empty(d->queue))
	{
		GelIOOpResult *res = gel_io_op_result_new(GEL_IO_OP_RESULT_RECURSE_TREE, d->tree);
		gel_io_op_success(self, res);
		g_free(res);

		return;
	}

	// Extract one from queue
	GFile *e = g_queue_pop_head(d->queue);

	// Start a new query
	d->current = gel_io_read_dir(e, d->attributes,
		recurse_dir_success_cb,
		recurse_dir_error_cb,
		self);
}

static void
recurse_dir_success_cb(GelIOOp *op, GFile *parent, GelIOOpResult *result, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GList *children = gel_io_op_result_get_object_list(result);

	// Add to result
	gel_io_recurse_tree_add_parent(RECURSE_DIR(self->_d)->tree, parent);
	gel_io_recurse_tree_add_children(RECURSE_DIR(self->_d)->tree, parent, children);

	// Launch suboperations for all directories in children
	GList *iter = children;
	while (iter)
	{
		GFileInfo *child_info = G_FILE_INFO(iter->data);

		// Got a symlink
		if (g_file_info_get_is_symlink(child_info))
		{
			GFile *link = gel_io_file_get_child_for_file_info(parent, child_info);
			const gchar *dest = g_file_info_get_symlink_target(child_info);
			GFile *b = g_file_resolve_relative_path(parent, dest);
			gel_warn("Got symlink  '%s' '%s' '%s'", g_file_get_uri(link), dest, g_file_get_uri(b));
/*
			gel_warn("Symlinks: %s", dest);
			GFile *b = g_file_resolve_relative_path(parent, dest);
			gel_warn("  goes to %s", g_file_get_uri(b));
			*/
		}
		else if (g_file_info_get_file_type(child_info) == G_FILE_TYPE_DIRECTORY)
		{
			GFile *child = gel_io_file_get_child_for_file_info(parent, child_info);
			g_queue_push_tail(RECURSE_DIR(self->_d)->queue, child);
		}

		iter = iter->next;
	}

	recurse_dir_run_queue(self);
}

static void
recurse_dir_error_cb(GelIOOp *op, GFile *parent, GError *error, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	if (self->error)
		self->error(self, parent, error, self->data);
	recurse_dir_run_queue(self);
}

// --
// List read
// --
typedef struct {
	GQueue       *queue;
	GCancellable *cancellable;
	GelIOOp      *subop;
	GList        *results;
	GSList       *sources;
	const gchar  *attributes;
} GelIOListRead;
#define LIST_READ(d) ((GelIOListRead*)d)

GelIOOp *
gel_io_op_list_read(GSList *uris, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data);
gboolean
list_read_destroy(GelIOOp *op);
gboolean
list_read_cancel(GelIOOp *op);
void
list_read_run_queue(GelIOOp *op);

gint
list_read_compare_files(GFile *a, GFile *b);
void
list_read_query_info_cb(GObject *source, GAsyncResult *res, gpointer data);
void
list_read_recurse_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data);
void
list_read_recurse_error_cb(GelIOOp *op, GFile *source, GError *err, gpointer data);
void
list_read_parse_tree(GelIOOp *op, GelIORecurseTree *tree, GFile *root);

GelIOOp *
gel_io_list_read(GSList *gfiles, const gchar *attributes,
	GelIOOpSuccessFunc success, GelIOOpErrorFunc error,
	gpointer data)
{
	GelIOOp *self = gel_io_op_new(NULL, NULL,
		success, error,
		list_read_cancel, list_read_destroy,
		data);
	self->_d = g_new0(GelIOListRead, 1);
	LIST_READ(self->_d)->queue  = g_queue_new();
	LIST_READ(self->_d)->sources = gfiles;
	LIST_READ(self->_d)->attributes = attributes;
	while (gfiles)
	{
		g_queue_push_tail(LIST_READ(self->_d)->queue, gfiles->data);
		gfiles = gfiles->next;
	}

	list_read_run_queue(self);
	return self;
}

GSList *
gel_io_list_read_get_sources(GelIOOp *self)
{
	return LIST_READ(self->_d)->sources;
}

gboolean
list_read_destroy(GelIOOp *op)
{
	if (!op->_d)
		return FALSE;

	list_read_cancel(op);

	if (LIST_READ(op->_d)->queue)
	{
		g_queue_free(LIST_READ(op->_d)->queue);
		LIST_READ(op->_d)->queue = NULL;
	}
	g_free(op->_d);
	return FALSE;
}

gboolean
list_read_cancel(GelIOOp *op)
{
	if (!op->_d)
		return FALSE;

	GelIOListRead *d = LIST_READ(op->_d);

	if (d->cancellable)
	{
		g_cancellable_cancel(d->cancellable);
		gel_free_and_invalidate(d->cancellable, NULL, g_object_unref);
	}
	if (d->subop)
	{
		gel_io_op_cancel(d->subop);
		gel_free_and_invalidate(d->subop, NULL, gel_io_op_unref);
	}
	if (d->results)
	{
		g_list_foreach(d->results, (GFunc) g_object_unref, NULL);
		g_list_free(d->results);
	}
	if (!g_queue_is_empty(d->queue))
	{
		g_queue_foreach(d->queue, (GFunc) g_object_unref, NULL);
		g_queue_clear(d->queue);
	}
	return FALSE; // Do normal cancel
}

void
list_read_run_queue(GelIOOp *op)
{
	GelIOListRead *d = LIST_READ(op->_d);

list_read_run_queue_retry:
	// Work is done?
	if (g_queue_is_empty(d->queue))
	{
		GelIOOpResult *res = gel_io_op_result_new(GEL_IO_OP_RESULT_OBJECT_LIST, d->results);
		gel_io_op_success(op, res);
		g_free(res);
		return;
	}

	GFile *file = g_queue_pop_head(d->queue);
	if (!G_IS_FILE(file))
	{
		gel_warn("gel_io_list_read needs GSList::GFile object");
		goto list_read_run_queue_retry;
	}
	d->cancellable = g_cancellable_new();
	g_file_query_info_async(file, d->attributes, G_FILE_QUERY_INFO_NONE,
		G_PRIORITY_DEFAULT, d->cancellable, list_read_query_info_cb,
		op);
}

gint
list_read_compare_files(GFile *a, GFile *b)
{
	gchar *a2 = g_file_get_uri(a);
	gchar *b2 = g_file_get_uri(b);

	gint ret = strcmp(a2, b2);

	g_free(a2);
	g_free(b2);

	return ret;
}

void
list_read_query_info_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);
	GelIOListRead *d = LIST_READ(self->_d);

	GError *err = NULL;

	GFileInfo *info = g_file_query_info_finish(G_FILE(source), res, &err);
	if (!info)
	{
		self->source = G_FILE(source);
		gel_io_op_error(self, err);
		g_error_free(err);
	}
	g_object_unref(d->cancellable);

	switch (g_file_info_get_file_type(info))
	{
	case G_FILE_TYPE_DIRECTORY:
		d->subop = gel_io_recurse_dir(G_FILE(source), d->attributes, 
			list_read_recurse_success_cb, list_read_recurse_error_cb,
			(gpointer) self);
		break;

	case G_FILE_TYPE_REGULAR:
		d->results = g_list_prepend(d->results, source);
		list_read_run_queue(self);
		break;

	default:
		list_read_run_queue(self);
		break;
	}
}

void
list_read_recurse_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *res, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	GelIORecurseTree *tree = gel_io_op_result_get_recurse_tree(res);
	list_read_parse_tree(self, tree, gel_io_recurse_tree_get_root(tree));

	gel_io_op_unref(op);
	list_read_run_queue(self);
}

void
list_read_recurse_error_cb(GelIOOp *op, GFile *source, GError *err, gpointer data)
{
	GelIOOp *self = GEL_IO_OP(data);

	self->source = source;
	gel_io_op_error(self, err);
	g_error_free(err);
}

void
list_read_parse_tree(GelIOOp *op, GelIORecurseTree *tree, GFile *root)
{
	GelIOListRead *d = LIST_READ(op->_d);
	GList *children = gel_io_recurse_tree_get_children(tree, root);
	GList *iter = children;
	GList *add = NULL;

	while (iter)
	{
		GFileInfo *info = G_FILE_INFO(iter->data);
		GFile *child = g_file_get_child(root, g_file_info_get_name(info));

		switch (g_file_info_get_file_type(G_FILE_INFO(iter->data)))
		{
		case G_FILE_TYPE_DIRECTORY:
			list_read_parse_tree(op, tree, child);
			g_object_unref(child);
			break;
		case G_FILE_TYPE_REGULAR:
			add = g_list_prepend(add, child);
			break;
		default:
			break;
		}
		iter = iter->next;
	}
	g_list_free(children);

	d->results = g_list_concat(d->results, g_list_sort(add, (GCompareFunc) list_read_compare_files));
}
