#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
#define EINA_PLUGIN_NAME "coverplus"
#define EINA_PLUGIN_DATA_TYPE CoverPlus

#include <gdk/gdk.h>
#include <gel/gel-io.h>
#include <eina/plugin.h>

typedef struct _CoverPlusInfolder CoverPlusInfolder;

typedef struct CoverPlus {
	// CoverPlusInfolder *infolder;
	gpointer dummy;
} CoverPlus;
#if 0

static gchar *coverplus_infolder_regex_str[] = {
	".*front.*\\.(jpe?g|png|gif)$",
	".*cover.*\\.(jpe?g|png|gif)$",
	".*folder.*\\.(jpe?g|png|gif)$",
	".*\\.(jpe?g|png|gif)$",
	NULL
};
struct _CoverPlusInfolder {
	EinaCover *cover;   // Ref to Cover
	GRegex    *regexes[4]; // Keep in sync with size of coverplus_infolder_regex_str
	GFile     *parent;
	GdkPixbuf *pixbuf;
	gchar     *name;
	gint       score;
	gboolean   running;

	GelIOOp *readdir_op, *readuri_op;
	/*
	GelIOAsyncReadDir *dir_reader;
	GelIOAsyncReadOp  *uri_reader;
	*/
};

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
 * Coverplus in-folder: Search for '/.*(front|cover).*\.(jpe?g,png,gif)$/i'
 */

CoverPlusInfolder*
coverplus_infolder_new(EinaCover *cover);
void
coverplus_infolder_reset(CoverPlusInfolder* self);
void
coverplus_infolder_free(CoverPlusInfolder* self);

/*
void
coverplus_infolder_readdir_error_cb(GelIOAsyncReadDir *obj, GError *error, gpointer data);
void
coverplus_infolder_readdir_finish_cb(GelIOAsyncReadDir *obj, GList *children, gpointer data);
void
coverplus_infolder_readuri_error_cb(GelIOAsyncReadOp *op, GError *error, gpointer data);
void
coverplus_infolder_readuri_finish_cb(GelIOAsyncReadOp *op, GByteArray *op_data, gpointer data);
*/
void
cover_plus_infolder_readdir_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
void
cover_plus_infolder_readdir_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);
void
cover_plus_infolder_readfile_succes_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
void
cover_plus_infolder_readfile_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);

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
	if (self->readdir_op)
	{
		gel_io_op_cancel(self->readdir_op);
		gel_free_and_invalidate(self->readdir_op, NULL, gel_io_op_unref);
	}
	if (self->readuri_op)
	{
		gel_io_op_cancel(self->readuri_op);
		gel_free_and_invalidate(self->readuri_op, NULL, gel_io_op_unref);
	}
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

	if (self->readdir_op)
	{
		gel_io_op_cancel(self->readdir_op);
		gel_free_and_invalidate(self->readdir_op, NULL, gel_io_op_unref);
	}
	if (self->readuri_op)
	{
		gel_io_op_cancel(self->readuri_op);
		gel_free_and_invalidate(self->readuri_op, NULL, gel_io_op_unref);
	}

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

	if (stream == NULL)
	{
		eina_cover_backend_fail(cover);
		return;
	}

	GFile *stream_file = g_file_new_for_uri(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	GFile *f = g_file_get_parent(stream_file);

	if (f == NULL)
	{
		gel_warn("Cannot get stream's parent");
		g_object_unref(stream_file);
		coverplus_infolder_finish(cover, self);
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
	self->readdir_op = gel_io_read_dir(f, "standard::*",
		cover_plus_infolder_readdir_success_cb, cover_plus_infolder_readdir_error_cb,
		self);
}

void
coverplus_infolder_finish(EinaCover *cover, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	GFile *f;
	gchar *uri;

	// Stop pending operations
	// gel_io_async_read_dir_cancel(self->dir_reader);
	gel_io_op_cancel(self->

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

		gel_io_async_read_op_fetch(self->uri_reader, f);

		g_free(uri);
	}

	else
	{
		eina_cover_backend_fail(self->cover);
	}
}

static void
coverplus_infolder_search_plugin_wrapper(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	coverplus_infolder_search(cover, stream, EINA_PLUGIN_DATA(data)->infolder);
}

static void
coverplus_infolder_finish_plugin_wrapper(EinaCover *cover, gpointer data)
{
	coverplus_infolder_finish(cover, EINA_PLUGIN_DATA(data)->infolder);
}

void
cover_plus_infolder_readdir_success_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	GList *children = gel_io_op_result_get_object_list(result);
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
	gel_io_op_unref(op);
	coverplus_infolder_finish(self->cover, self);
}

void
coverplus_infolder_readuri_error_cb(GelIOAsyncReadOp *op, GError *error, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	gel_error("Error fetching URI: %s", error->message);
	gel_io_op_unref(op);
	coverplus_infolder_finish(self->cover, self);
}

void
cover_plus_infolder_readdir_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	gel_warn("Got error while fetching children: %s", error->message);
	coverplus_infolder_finish(self->cover, self);
}

void
cover_plus_infolder_readfile_succes_cb(GelIOOp *op, GFile *source, GelIOOpResult *result, gpointer data);
void
cover_plus_infolder_readfile_error_cb(GelIOOp *op, GFile *source, GError *error, gpointer data);
#if 0
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

	g_file_set_contents("/tmp/eina.cover", (gchar*) op_data->data, op_data->len, NULL);
	self->pixbuf = gdk_pixbuf_new_from_file("/tmp/eina.cover", &err);
	if (err != NULL)
	{
		gel_error("Cannot load file: %s", err->message);
		g_error_free(err);
	}
	coverplus_infolder_finish(self->cover, self);
}
#endif
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
coverplus_exit(EinaPlugin *plugin, GError **error)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);

	eina_cover_remove_backend(cover, "coverplus-banshee");
	// eina_cover_remove_backend(cover, "coverplus-infolder");

	// coverplus_infolder_free(EINA_PLUGIN_DATA(plugin)->infolder);
	g_free(EINA_PLUGIN_DATA(plugin));

	return TRUE;
}

gboolean
coverplus_init(EinaPlugin *plugin, GError **error)
{
	CoverPlus *self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);
	EinaCover *cover = eina_plugin_get_player_cover(plugin);

	plugin->data = self;
	/*
	self->infolder = coverplus_infolder_new(eina_plugin_get_player_cover(plugin));
	self->infolder->cover = cover;
	*/
	eina_cover_add_backend(cover, "coverplus-banshee",
		coverplus_banshee_search, NULL, NULL);
	/*
	eina_cover_add_backend(cover, "coverplus-infolder",
		coverplus_infolder_search_plugin_wrapper,
		coverplus_infolder_finish_plugin_wrapper,
		plugin);
	*/
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

