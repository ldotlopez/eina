/*
 * plugins/coverplus/infolder-async.c
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

#define GEL_DOMAIN "Eina::Plugin::CoverPlus::Infolder"

#include <config.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/plugin.h>

static gchar *coverplus_infolder_regex_str[] = {
	".*front.*\\.(jpe?g|png|gif)$",
	".*cover.*\\.(jpe?g|png|gif)$",
	".*folder.*\\.(jpe?g|png|gif)$",
	".*\\.(jpe?g|png|gif)$",
	NULL
};

struct _CoverPlusInfolder {
	EinaArtwork  *cover;
	GRegex       *regexes[4]; // Keep in sync with size of coverplus_infolder_regex_str
	GelIOOp      *async_op;
	GCancellable *cancellable;
	gint          score;
};

void
coverplus_infolder_reset(CoverPlusInfolder *self)
{
}

#ifdef ASYNC_OP
void
coverplus_infolder_readdir_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
void
coverplus_infolder_readdir_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);
void
coverplus_load_contents_cb(GObject *source, GAsyncResult *res, gpointer data);
#endif

CoverPlusInfolder *
coverplus_infolder_new(EinaArtwork *cover)
{
	CoverPlusInfolder *self = g_new0(CoverPlusInfolder, 1);

	self->cover = cover;
	self->score = G_MAXINT;

	// Compile regexes
	gint i;
	for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
	{
		GError *err = NULL;
		self->regexes[i] = g_regex_new(coverplus_infolder_regex_str[i],
			G_REGEX_CASELESS|G_REGEX_DOTALL|G_REGEX_DOLLAR_ENDONLY|G_REGEX_OPTIMIZE|G_REGEX_NO_AUTO_CAPTURE,
			0, &err);
		if (!self->regexes[i])
		{
			gel_error("Cannot compile regex '%s': '%s'", coverplus_infolder_regex_str[i], err->message);
			g_error_free(err);
			coverplus_infolder_destroy(self);
			return NULL;
		}
	}

	return self;
}

void
coverplus_infolder_destroy(CoverPlusInfolder *self)
{
	// Stop any pending operation
	coverplus_infolder_reset(self);

	// Free regexes
	gint i;
	for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
		if (self->regexes[i])
			g_regex_unref(self->regexes[i]);

	// Bye
	g_free(self);
}

void
coverplus_infolder_reset(CoverPlusInfolder *self)
{
	// Send result to EinaArtwork object if no operations in progress and
	// a score < G_MAXINT
	if (!self->async_op && !self->cancellable && (self->score < G_MAXINT))
	{
		eina_artwork_provider_success(self->cover, G_TYPE_STRING, self->result);
	}
	else
	{
		eina_artwork_provider_fail(self->cover);
	}

	// Consistence check
	if (self->async_op && self->cancellable)
		gel_error("Async operation and cancellable active");

	// Cancel GelIOOp
	if (self->async_op)
	{
		gel_io_op_cancel(self->async_op);
		gel_io_op_unref(self->async_op);
		self->async_op = NULL;
		eina_artwork_provider_fail(self->cover);
	}

	// Cancell GIO op
	if (self->cancellable)
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
		eina_artwork_provider_fail(self->cover);
	}

	// Reset score
	self->score = G_MAXINT;
}

void
coverplus_infolder_search(CoverPlusInfolder *self, const LomoStream *stream)
{
	// Ensure a good state
	coverplus_infolder_reset(self);

	// Launch readdir op
	gchar *dirname = g_path_get_dirname(lomo_stream_get_tag(stream , LOMO_TAG_URI));
	self->async_op = gel_io_read_dir(g_file_new_for_uri(dirname), "standard::*",
		coverplus_infolder_readdir_success_cb, coverplus_infolder_readdir_error_cb,
		self);
	g_free(dirname);
}

void
coverplus_infolder_cancel(CoverPlusInfolder *self)
{
	coverplus_infolder_reset(self);
}

// --
// Wrappers
// --
void
// coverplus_infolder_search_wrapper(EinaCover *plugin, const LomoStream *stream, gpointer data)
coverplus_infolder_search_wrapper(EinaArtwork *artwork, LomoStream *stream, gpointer data)
{
	coverplus_infolder_search(EINA_PLUGIN_DATA(data)->infolder, stream);
}

void
// coverplus_infolder_cancel_wrapper(EinaCover *cover, gpointer data)
coverplus_infolder_cancel_wrapper(EinaArtwork *artwork, gpointer data)
{
	coverplus_infolder_cancel(EINA_PLUGIN_DATA(data)->infolder);
}

// --
// Callbacks
// --
void
coverplus_infolder_readdir_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder*) data;
	GList *iter, *children;

	gchar *match = NULL;
	iter = children = gel_io_op_result_get_object_list(result);
	while (iter)
	{
		GFileInfo *info = G_FILE_INFO(iter->data);
		const gchar *name = g_file_info_get_name(info);
		
		gint i;
		for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
		{
			if (g_regex_match(self->regexes[i], name, 0, NULL) && (i < self->score))
			{
				gel_free_and_invalidate(match, NULL, g_free);
				match = g_strdup(name);
				self->score = i;
				break;
			}
		}	
		iter = iter->next;
	}
	g_list_free(children);

	if (!match)
	{
		coverplus_infolder_reset(self);
		return;
	}

	gel_io_op_unref(self->async_op);
	self->async_op = NULL;

	// Async read of file
	self->cancellable = g_cancellable_new();
	g_file_load_contents_async(g_file_get_child(source, match),
		self->cancellable,
		coverplus_load_contents_cb, 
		self);
}

void
coverplus_infolder_readdir_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data)
{
	gchar *uri = g_file_get_uri(source);
	gel_error("Cannot readdir %s: %s", source, error->message);
	g_free(uri);

	coverplus_infolder_reset((CoverPlusInfolder *) data);
}

void
coverplus_load_contents_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	char *contents = NULL;
	gsize size;
	GError *err = NULL;

	if (!g_file_load_contents_finish(G_FILE(source), res, &contents, &size, NULL, &err))
	{
		gel_error("Cannot load file: %s", err->message);
		g_error_free(err);
		coverplus_infolder_reset((CoverPlusInfolder *) data);
		return;
	}
	g_object_unref(source);

	gchar *tmpfile = g_build_filename(g_get_home_dir(), "." PACKAGE, "tmpcover", NULL);
	if (!g_file_set_contents(tmpfile, contents, size, &err))
	{
		gel_error("Cannot create pixbuf: %s", err->message);
		g_error_free(err);
		coverplus_infolder_cancel((CoverPlusInfolder *) data);
	}
	else
		// eina_cover_backend_success(((CoverPlusInfolder *) data)->cover, G_TYPE_STRING, tmpfile);
		eina_artwork_provider_success(((CoverPlusInfolder *) data)->cover, G_TYPE_STRING, tmpfile);

	g_unlink(tmpfile);
	g_free(tmpfile);
}

