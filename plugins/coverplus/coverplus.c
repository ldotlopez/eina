#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/iface.h>
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

/*
 * Coverplus in-folder: Search for '/.*(front|cover).*\.(jpe?g,png,gif)$/i'
 */
static gchar *coverplus_infolder_regex_str[] = {
	".*front.*\\.(jpe?g|png|gif)$",
	".*cover.*\\.(jpe?g|png|gif)$",
	".*folder.*\\.(jpe?g|png|gif)$",
	".*\\.(jpe?g|png|gif)$",
	NULL
};

typedef struct {
	EinaCover *cover;   // Ref to Cover
	GRegex    *regexes[4]; // Keep in sync with size of coverplus_infolder_regex_str
	GFile     *parent;
	GdkPixbuf *pixbuf;
	gchar     *name;
	gint       score;
	gboolean   running;

	GelIOAsyncReadDir *dir_reader;
	GelIOAsyncReadOp  *uri_reader;
} CoverPlusInfolder;

CoverPlusInfolder*
coverplus_infolder_new(EinaCover *cover);
void
coverplus_infolder_reset(CoverPlusInfolder* self);
void
coverplus_infolder_free(CoverPlusInfolder* self);

void
coverplus_infolder_readdir_error_cb(GelIOAsyncReadDir *obj, GError *error, gpointer data);
void
coverplus_infolder_readdir_finish_cb(GelIOAsyncReadDir *obj, GList *children, gpointer data);
void
coverplus_infolder_readuri_error_cb(GelIOAsyncReadOp *op, GError *error, gpointer data);
void
coverplus_infolder_readuri_finish_cb(GelIOAsyncReadOp *op, GByteArray *op_data, gpointer data);

void
coverplus_infolder_search(EinaCover *cover, const LomoStream *stream, gpointer data);
void
coverplus_infolder_finish(EinaCover *cover, gpointer data);

CoverPlusInfolder*
coverplus_infolder_new(EinaCover *cover)
{
	GError *err = NULL;
	gint i;

	CoverPlusInfolder *self = g_new0(CoverPlusInfolder, 1);

	// Create regexes
	for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
	{
		self->regexes[i] = g_regex_new(coverplus_infolder_regex_str[i],
			G_REGEX_CASELESS|G_REGEX_DOTALL|G_REGEX_DOLLAR_ENDONLY|G_REGEX_OPTIMIZE|G_REGEX_NO_AUTO_CAPTURE,
			0,
			&err);
		if (err != NULL)
		{
			gel_error("Cannot create regex: '%s'", err->message);
			g_error_free(err);
		}
	}

	// Attach EinaCover
	self->cover = cover;
	g_object_ref(self->cover);

	// Create GelIO dir reader 
	self->dir_reader = gel_io_async_read_dir_new();
	self->uri_reader = gel_io_async_read_op_new();

	g_signal_connect(self->dir_reader, "error",
	(GCallback) coverplus_infolder_readdir_error_cb, self);
	g_signal_connect(self->dir_reader, "finish",
	(GCallback) coverplus_infolder_readdir_finish_cb, self);

	g_signal_connect(self->uri_reader, "error",
	(GCallback) coverplus_infolder_readuri_error_cb, self);
	g_signal_connect(self->uri_reader, "finish",
	(GCallback) coverplus_infolder_readuri_finish_cb, self);

	// Those fields are by default set correctly
	self->running = FALSE;
	self->parent  = NULL;
	self->name    = NULL;
	self->pixbuf  = NULL;
	self->score   = G_MAXINT;

	return self;
}

void
coverplus_infolder_reset(CoverPlusInfolder* self)
{
	// Cancel operations
	gel_io_async_read_dir_cancel(self->dir_reader);
	self->running = FALSE;

	// Free parent and name
	gel_free_and_invalidate(self->parent, NULL, g_object_unref);
	gel_free_and_invalidate(self->name,   NULL, g_free);
	gel_free_and_invalidate(self->pixbuf, NULL, g_object_unref);

	// Reset score
	self->score = G_MAXINT;
}

void
coverplus_infolder_free(CoverPlusInfolder* self)
{
	gint i;

	// Free regexes
	for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
		if (self->regexes[i])
			g_object_unref(self->regexes[i]);

	gel_io_async_read_dir_cancel(self->dir_reader);
	g_object_unref(self->dir_reader);

	// Free parent and name
	gel_free_and_invalidate(self->parent, NULL, g_object_unref);
	gel_free_and_invalidate(self->name,   NULL, g_free);
	gel_free_and_invalidate(self->pixbuf, NULL, g_object_unref);

	// Unref cover
	g_object_unref(self->cover);

	g_free(self);
}

void
coverplus_infolder_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;

	GFile *stream_file = g_file_new_for_uri(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	GFile *f = g_file_get_parent(stream_file);

	if (f == NULL)
	{
		gel_warn("Cannot get stream's parent");
		g_object_unref(stream_file);
		coverplus_infolder_finish(self->cover, self);
		return;
	}
	g_object_unref(stream_file);

	if ((self->parent != NULL) && g_file_equal(self->parent, f))
	{
		g_object_unref(f);
		coverplus_infolder_finish(self->cover, self);
		return;
	}

	coverplus_infolder_reset(self);

	self->parent = f;
	gel_io_async_read_dir_scan(self->dir_reader, self->parent, G_FILE_ATTRIBUTE_STANDARD_NAME);
}

void
coverplus_infolder_finish(EinaCover *cover, gpointer data)
{
	CoverPlusInfolder* self = (CoverPlusInfolder*) data;
	GFile *f;
	gchar *uri;

	// Stop pending operations
	gel_io_async_read_dir_cancel(self->dir_reader);

	// Dont free regexes, they are shared between multiple searches

	// Send results now, if uri != NULL we got positive results, build a
	// correct uri and send to cover, else, send a fail message

	if ((self->pixbuf != NULL) && GDK_IS_PIXBUF(self->pixbuf))
	{
		eina_cover_backend_success(cover, GDK_TYPE_PIXBUF, self->pixbuf);
	}

	else if (self->name != NULL)
	{
		f   = g_file_get_child(self->parent, self->name);
		uri = g_file_get_uri(f);

		gel_warn("Got cover: %s, launch fetch", uri);
		gel_io_async_read_op_fetch(self->uri_reader, f);

		g_free(uri);
	}

	else
	{
		gel_error("No cover found");
		eina_cover_backend_fail(self->cover);
	}
}

void
coverplus_infolder_readdir_error_cb(GelIOAsyncReadDir *obj, GError *error, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	gel_warn("Got error while fetching children: %s", error->message);
	coverplus_infolder_finish(self->cover, self);
}

void
coverplus_infolder_readdir_finish_cb(GelIOAsyncReadDir *obj, GList *children, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	GList       *iter;
	gint         i;
	GFileInfo   *info;
	const gchar *name;

	iter = children;
	while (iter)
	{
		info = G_FILE_INFO(iter->data);
		name = g_file_info_get_name(info);

		for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
		{
			if (g_regex_match(self->regexes[i], name, 0, NULL) && (i < self->score))
			{
				gel_free_and_invalidate(self->name, NULL, g_free);
				self->name = g_strdup(name);
				self->score = i;
				break;
			}
		}	
		iter = iter->next;
	}
	coverplus_infolder_finish(self->cover, self);
}

void
coverplus_infolder_readuri_error_cb(GelIOAsyncReadOp *op, GError *error, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	gel_error("Error fetching URI: %s", error->message);
	coverplus_infolder_finish(self->cover, self);
}

void
coverplus_infolder_readuri_finish_cb(GelIOAsyncReadOp *op, GByteArray *op_data,  gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	GError *err = NULL;
	gel_warn("Readed %d bytes from URI", op_data->len);

	g_file_set_contents("/tmp/eina.cover", (gchar*) op_data->data, op_data->len, NULL);
	self->pixbuf = gdk_pixbuf_new_from_file("/tmp/eina.cover", &err);
	if (err != NULL)
	{
		gel_error("Cannot load file: %s", err->message);
		g_error_free(err);
	}
	coverplus_infolder_finish(self->cover, self);
}

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
	gel_warn("%s", path);
	g_string_free(str, TRUE);

	if (g_file_test(path, G_FILE_TEST_IS_REGULAR|G_FILE_TEST_EXISTS))
	{
		gel_warn("Success!");
		eina_cover_backend_success(cover, G_TYPE_STRING, path);
	}
	else
	{
		gel_warn("Fail");
		eina_cover_backend_fail(cover);
	}

	g_free(path);
}

/*
 * Main
 */
EINA_PLUGIN_FUNC gboolean
coverplus_exit(EinaPlugin *self)
{
	if (coverplus_timeout_test_source_id > 0)
		g_source_remove(coverplus_timeout_test_source_id);
	eina_plugin_free(self);
	return TRUE;
}

EINA_PLUGIN_FUNC EinaPlugin*
coverplus_init(GelHub *app, EinaIFace *iface)
{
	EinaCover *cover;

	EinaPlugin *self = eina_plugin_new(iface,
		"coverplus", "cover", NULL, coverplus_exit,
		NULL, NULL, NULL);
	
	cover = eina_iface_get_cover(iface);
	if (cover == NULL)
	{
		gel_error("Cannot get a valid cover object, aborting");
		eina_plugin_free(self);
		return NULL;
	}

	eina_cover_add_backend(cover, "coverplus-banshee",
		coverplus_banshee_search, NULL, NULL);
	eina_cover_add_backend(cover, "coverplus-infolder",
 		coverplus_infolder_search, coverplus_infolder_finish,
		coverplus_infolder_new(cover));
	return self;
}

