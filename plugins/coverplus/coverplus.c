#define GEL_DOMAIN "Eina::Plugin::CoverPlus"
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
	".*\\.(jpe?g|png|gif)$",
	NULL
};

typedef struct {
	EinaCover    *cover;   // Ref to Cover
	GRegex *regexes[4]; // Keep in sync with size of coverplus_infolder_regex_str
	GCancellable *cancellable; // Allow to cancel operations
	GFile *parent;
	gchar *name;  
	gint   score;
	gboolean running;
} CoverPlusInfolder;


CoverPlusInfolder*
coverplus_infolder_new(EinaCover *cover);
void
coverplus_infolder_reset(CoverPlusInfolder* self);
void
coverplus_infolder_free(CoverPlusInfolder* self);
void
coverplus_infolder_search(EinaCover *cover, const LomoStream *stream, gpointer data);
void
coverplus_infolder_finish(EinaCover *cover, gpointer data);
void
coverplus_infolder_enumerate_children_cb(GObject *source, GAsyncResult *res, gpointer data);
void
coverplus_infolder_next_files_cb(GObject *source, GAsyncResult *res, gpointer data);
void
coverplus_infolder_enumerate_close_cb(GObject *source, GAsyncResult *res, gpointer data);

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

	// Create cancellable
	self->cancellable = g_cancellable_new();

	// Those fields are by default set correctly
	self->running = FALSE;
	self->parent  = NULL;
	self->name    = NULL;
	self->score   = G_MAXINT;

	return self;
}

void
coverplus_infolder_reset(CoverPlusInfolder* self)
{
	// Cancel operations
	if (self->running && !g_cancellable_is_cancelled(self->cancellable))
	{
		g_cancellable_cancel(self->cancellable);
		g_cancellable_reset(self->cancellable);
	}
	self->running = FALSE;

	// Free parent and name
	gel_free_and_invalidate(self->parent, NULL, g_object_unref);
	gel_free_and_invalidate(self->name,   NULL, g_free);

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

	// Free cancellable
	if (self->running && !g_cancellable_is_cancelled(self->cancellable))
	{
		g_cancellable_cancel(self->cancellable);
		g_object_unref(self->cancellable);
		self->running = FALSE;
	}

	// Free parent and name
	gel_free_and_invalidate(self->parent, NULL, g_object_unref);
	gel_free_and_invalidate(self->name,   NULL, g_free);

	// Unref cover
	g_object_unref(self->cover);

	g_free(self);
}

void
coverplus_infolder_search(EinaCover *cover, const LomoStream *stream, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;

	GFile *stream_file = g_file_new_for_uri(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	GFile *f           = g_file_get_parent(stream_file);
	if (f == NULL)
	{
		gel_warn("Cannot get stream's parent");
		coverplus_infolder_finish(self->cover, self);
		return;
	}

	if ((self->parent != NULL) && (g_file_equal(self->parent,f)))
	{
		gel_warn("New stream is brother from previous one, no need to search");
		g_object_unref(f);
		coverplus_infolder_finish(self->cover, self);
		return;
	}

	self->parent = f;
	g_object_ref(self->parent);
	g_file_enumerate_children_async(self->parent,
		G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_QUERY_INFO_NONE,
		G_PRIORITY_DEFAULT,
		self->cancellable,
		coverplus_infolder_enumerate_children_cb,
		(gpointer) self);
}

void
coverplus_infolder_finish(EinaCover *cover, gpointer data)
{
	CoverPlusInfolder* self = (CoverPlusInfolder*) data;
	GFile *cover_file;
	gchar *cover_uri;

	// Ok, we finish
	
	// First go to stable state, this function can be called to exit from
	// an error or to cleanup after an correct execution.
	
	// Stop pending operations
	g_cancellable_reset(self->cancellable);

	// Dont free regexes, they are shared between multiple searches

	// baseurl is not freeded, its used to optimize searches, if previous
	// baseurl is equal to new baseurl in next search results willbe the same

	// uri has the same schema that baseurl

	// Send results now, if uri != NULL we got positive results, build a
	// correct uri and send to cover, else, send a fail message
	if (self->name != NULL)
	{
		cover_file = g_file_get_child(self->parent, self->name);
		cover_uri  = g_file_get_uri(cover_file);
		gel_warn("Got cover: '%s'", cover_uri);
		g_free(cover_uri);
		g_object_unref(cover_file);
	}
}

void
coverplus_infolder_enumerate_children_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;
	GFile           *file;
	GFileEnumerator *enumerator;
	GError          *error = NULL;
	gchar           *uri;

	file       = G_FILE(source);
	enumerator = g_file_enumerate_children_finish(file, res, &error);

	if (enumerator == NULL)
	{
		uri = g_file_get_uri(file);
		gel_error("Cannot enumerate children of '%s': %s", uri, error->message);
		g_free(uri);
		g_error_free(error);
		g_object_unref(source);
		eina_cover_backend_fail(self->cover);
		coverplus_infolder_free(self);
		return;
	}
	g_object_unref(source);

	g_file_enumerator_next_files_async(enumerator,
		4,
		G_PRIORITY_DEFAULT,
		self->cancellable,
		coverplus_infolder_next_files_cb,
		(gpointer) self);
}

void
coverplus_infolder_next_files_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	CoverPlusInfolder *self = (CoverPlusInfolder *) data;

	GError *error = NULL;
	GList  *list, *l;

	const gchar *fileinfo_name;
	gint i;

	list = l = g_file_enumerator_next_files_finish(G_FILE_ENUMERATOR(source), res, &error);
	if (error != NULL)
	{
		gel_error("Error while getting children.");
		gel_glist_free(list, (GFunc) g_object_unref, NULL);
		gel_glist_free(l, (GFunc) NULL, NULL);
		eina_cover_backend_fail(self->cover);
		return;
	}

	if (l == NULL)
	{
		g_file_enumerator_close_async(G_FILE_ENUMERATOR(source),
			G_PRIORITY_DEFAULT,
			self->cancellable,
			coverplus_infolder_enumerate_close_cb,
			(gpointer) self);
		return;
	}

	while (l)
	{
		if (!G_FILE_INFO(l->data))
		{
			gel_warn("Object gotten '%p' is not a GFileInfo, is a %s", l->data, G_OBJECT_TYPE_NAME(l->data));
			l = l->next;
			continue;
		}

		fileinfo_name = g_file_info_get_name(G_FILE_INFO(l->data));
		for (i = 0; coverplus_infolder_regex_str[i] != NULL; i++)
		{
			if (self->regexes[i] == NULL)
				continue;

			if (g_regex_match(self->regexes[i], fileinfo_name, 0, NULL))
			{
				// Got match
				if (i < self->score)
				{
					self->score = i;
					if (self->name != NULL)
						g_free(self->name);
					self->name = g_strdup(fileinfo_name);
				}

				break;
			}
		}
		l = l->next;
	}

	gel_glist_free(l, (GFunc) g_object_unref, NULL);
	g_file_enumerator_next_files_async(G_FILE_ENUMERATOR(source),
		4,
		G_PRIORITY_DEFAULT,
		self->cancellable,
		coverplus_infolder_next_files_cb,
		(gpointer) self);
}

void
coverplus_infolder_enumerate_close_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	CoverPlusInfolder* self = (CoverPlusInfolder *) data;

	GError *error = NULL;
	gboolean r = g_file_enumerator_close_finish(G_FILE_ENUMERATOR(source), res, &error);
	if (r == FALSE)
	{
		gel_error("Cannot close");
	}
	coverplus_infolder_finish(self->cover, self);
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

	eina_cover_add_backend(cover, "coverplus-timeout-test",
		coverplus_timeout_test_search, coverplus_timeout_test_cancel, NULL);

	eina_cover_add_backend(cover, "coverplus-infolder",
 		coverplus_infolder_search, coverplus_infolder_finish,
		coverplus_infolder_new(cover));
	return self;
}

