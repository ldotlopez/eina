#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#define EINA_PLUGIN_NAME "coverplus"
#define EINA_PLUGIN_DATA_TYPE CoverPlus

#include <config.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/plugin.h>

typedef struct _CoverPlusInfolder CoverPlusInfolder;

typedef struct CoverPlus {
	CoverPlusInfolder *infolder;
} CoverPlus;

static gchar *coverplus_infolder_regex_str[] = {
	".*front.*\\.(jpe?g|png|gif)$",
	".*cover.*\\.(jpe?g|png|gif)$",
	".*folder.*\\.(jpe?g|png|gif)$",
	".*\\.(jpe?g|png|gif)$",
	NULL
};

struct _CoverPlusInfolder {
	EinaCover    *cover;
	GRegex       *regexes[4]; // Keep in sync with size of coverplus_infolder_regex_str
	GelIOOp      *async_op;
	GCancellable *cancellable;
	gint          score;
};

CoverPlusInfolder *
coverplus_infolder_new(EinaCover *cover);
void
coverplus_infolder_destroy(CoverPlusInfolder *self);
void
coverplus_infolder_reset(CoverPlusInfolder *self);

void
coverplus_infolder_readdir_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
void
coverplus_infolder_readdir_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);
void
coverplus_load_contents_cb(GObject *source, GAsyncResult *res, gpointer data);

CoverPlusInfolder *
coverplus_infolder_new(EinaCover *cover)
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
	// Consistence check
	if (self->async_op && self->cancellable)
		gel_error("Async operation and cancellable active");

	// Cancel GelIOOp
	if (self->async_op)
	{
		gel_io_op_cancel(self->async_op);
		gel_io_op_unref(self->async_op);
		self->async_op = NULL;
		eina_cover_backend_fail(self->cover);
	}

	// Cancell GIO op
	if (self->cancellable)
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->cancellable = NULL;
		eina_cover_backend_fail(self->cover);
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
coverplus_infolder_search_wrapper(EinaCover *plugin, const LomoStream *stream, gpointer data)
{
	coverplus_infolder_search(EINA_PLUGIN_DATA(data)->infolder, stream);
}

void
coverplus_infolder_cancel_wrapper(EinaCover *cover, gpointer data)
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

	gchar *tmpfile = g_build_filename(g_get_home_dir(), "." PACKAGE_NAME, "tmpcover", NULL);
	if (!g_file_set_contents(tmpfile, contents, size, &err))
	{
		gel_error("Cannot create pixbuf: %s", err->message);
		g_error_free(err);
		coverplus_infolder_cancel((CoverPlusInfolder *) data);
	}
	else
		eina_cover_backend_success(((CoverPlusInfolder *) data)->cover, G_TYPE_STRING, tmpfile);

	g_unlink(tmpfile);
	g_free(tmpfile);
}

#if 0
/*
 * Timeout-based cover backend. For testing
 */
static guint coverplus_timeout_test_source_id = 0;
void
coverplus_timeout_test_search(EinaCover *cover, const LomoStream *stream, gpointer data);
void
coverplus_timeout_test_cancel(EinaCover *cover, gpointer data);
static
gboolean coverplus_timeout_test_result(gpointer data);

// Timeout-test
void
coverplus_timeout_test_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	 coverplus_timeout_test_source_id = g_timeout_add(5000, (GSourceFunc) coverplus_timeout_test_result, cover);
}

void
coverplus_timeout_test_cancel(EinaCover *cover, gpointer data)
{
	if (coverplus_timeout_test_source_id <= 0)
		return;

	g_source_remove(coverplus_timeout_test_source_id);
	coverplus_timeout_test_source_id = 0;
}

static gboolean
coverplus_timeout_test_result(gpointer data)
{
	EinaCover *cover = EINA_COVER(data);
	coverplus_timeout_test_source_id = 0;
	eina_cover_backend_fail(cover);
	return FALSE;
}
#endif

/*
 * Banshee covers
 */
void
coverplus_banshee_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	GString *str;
	gchar *path = NULL;
	gint i, j;
	gchar *input[3] = {
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ARTIST), -1),
		g_utf8_strdown(lomo_stream_get_tag(stream, LOMO_TAG_ALBUM), -1),
		NULL
	};

	str = g_string_new(NULL);

	for (i = 0; input[i] != NULL; i++)
	{
		for (j = 0; input[i][j] != '\0'; j++)
		{
			if (g_ascii_isalnum(input[i][j]))
				str = g_string_append_c(str, input[i][j]);
		}
		if (i == 0)
			str = g_string_append_c(str, '-');
		g_free(input[i]);
	}
	str = g_string_append(str, ".jpg");

	path = g_build_filename(g_get_home_dir(), ".config", "banshee", "covers", str->str, NULL);
	g_string_free(str, TRUE);

	if (g_file_test(path, G_FILE_TEST_IS_REGULAR|G_FILE_TEST_EXISTS))
		eina_cover_backend_success(cover, G_TYPE_STRING, path);
	else
		eina_cover_backend_fail(cover);

	g_free(path);
}

// --
// Main
// --
gboolean
coverplus_init(EinaPlugin *plugin, GError **error)
{
	CoverPlus *self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);
	EinaCover *cover = eina_plugin_get_player_cover(plugin);

	plugin->data = self;
	self->infolder = coverplus_infolder_new(cover);

	// eina_cover_add_backend(cover, "coverplus-banshee", coverplus_banshee_search, NULL, NULL);

	eina_cover_add_backend(cover, "coverplus-infolder",
		coverplus_infolder_search_wrapper,
		coverplus_infolder_cancel_wrapper,
		plugin);
	return TRUE;
}

gboolean
coverplus_exit(EinaPlugin *plugin, GError **error)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);

	// eina_cover_remove_backend(cover, "coverplus-banshee");
	eina_cover_remove_backend(cover, "coverplus-infolder");

	coverplus_infolder_destroy(EINA_PLUGIN_DATA(plugin)->infolder);
	g_free(EINA_PLUGIN_DATA(plugin));

	return TRUE;
}

G_MODULE_EXPORT EinaPlugin coverplus_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Cover plus"),
	"0.7.0",
	N_("Enhace your covers"),
	N_("bla ble bli"),
	NULL,
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net/",

	coverplus_init, coverplus_exit,

	NULL, NULL
};

