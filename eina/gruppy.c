#include <gel/gel.h>
#include "gruppy.h"

struct _GruppyDir {
	GCancellable *cancellable;
	GList        *children;

	GruppyDirSuccessFunc   success;
	GruppyDirErrorFunc     error;
	GruppyDirCancelledFunc cancelled;
	gpointer data;
	GFreeFunc free_func;
};

static void
enumerate_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
static void
close_cb(GObject *source, GAsyncResult *res, gpointer data);

GruppyDir*
gruppy_dir_read_full(GFile *file, const gchar *attributes,
	GruppyDirSuccessFunc   success,
	GruppyDirErrorFunc     error,
	GruppyDirCancelledFunc cancelled,
	gpointer data,
	GFreeFunc free_func)
{
	GruppyDir *self = g_new0(GruppyDir, 1);
	self->success   = success;
	self->error     = error;
	self->cancelled = cancelled;

	self->cancellable = g_cancellable_new();

	self->data = data;
	self->free_func = free_func;

    g_file_enumerate_children_async(file,
		attributes, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
		self->cancellable,
		enumerate_cb,
		self);

	return self;
}

void
gruppy_dir_cancel(GruppyDir *gruppy)
{
	g_cancellable_cancel(gruppy->cancellable);

	g_object_unref(gruppy->cancellable);
	gel_glist_free(gruppy->children, (GFunc) g_object_unref, NULL);
}

void
gruppy_dir_close(GruppyDir *gruppy)
{
	if (!g_cancellable_is_cancelled(gruppy->cancellable))
		g_cancellable_cancel(gruppy->cancellable);
	g_object_unref(gruppy->cancellable);
	
	gel_glist_free(gruppy->children, (GFunc) g_object_unref, NULL);
	gruppy->free_func(gruppy->data);
	g_free(gruppy);
}

void
gruppy_dir_set_data (GruppyDir *gruppy, gpointer data)
{
	if (gruppy->data && gruppy->free_func)
		gruppy->free_func(gruppy->data);
	gruppy->data = data;
}

gpointer
gruppy_dir_get_data (GruppyDir *gruppy)
{
	return gruppy->data;
}

static void
enumerate_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GruppyDir *gruppy = (GruppyDir *) data;

	GFile           *file;
	GFileEnumerator *enumerator;
	GError          *err = NULL;

	file = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(file, res, &err);
	if (enumerator == NULL)
	{
		gruppy->error(gruppy, err, gruppy->data);
		g_error_free(err);
		return;
	}

	g_file_enumerator_next_files_async(enumerator,
		4, G_PRIORITY_DEFAULT,
		gruppy->cancellable,
		next_files_cb,
		(gpointer) gruppy);
}

static void
next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GruppyDir       *gruppy;
	GFileEnumerator *enumerator;
	GList           *items;
	GError          *err = NULL;

	enumerator = G_FILE_ENUMERATOR(source);
	items = g_file_enumerator_next_files_finish(enumerator, res, &err);

	if (err != NULL)
	{
		g_object_unref(enumerator);
		gel_glist_free(items, (GFunc) g_object_unref, NULL);
		gruppy->error(gruppy, err, gruppy->data);
		g_error_free(err);
		return;
	}

	if (g_list_length(items) > 0)
	{
		gruppy->children = g_list_concat(gruppy->children, items);

		g_file_enumerator_next_files_async(enumerator,
			4, G_PRIORITY_DEFAULT,
			gruppy->cancellable,
			next_files_cb,
			(gpointer) data);
	}
	else
	{
		g_file_enumerator_close_async(enumerator,
			G_PRIORITY_DEFAULT,
			gruppy->cancellable,
			close_cb,
			(gpointer) data);
	}
}

static void
close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GruppyDir *gruppy = (GruppyDir *) data;

	GFileEnumerator *enumerator;
	gboolean         result;
	GError          *err = NULL;

	enumerator = G_FILE_ENUMERATOR(source);
	result = g_file_enumerator_close_finish(enumerator, res, &err);

	g_object_unref(enumerator);

	if (result == FALSE)
	{
		gruppy->error(gruppy, err, gruppy->data);
		g_error_free(err);
	}
	else
	{
		gruppy->success(gruppy, gruppy->children, gruppy->data);
	}
}

