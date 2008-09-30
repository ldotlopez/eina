#define GEL_DOMAIN "Eina::Plugin::LastFMCover"
#define EINA_PLUGIN_DATA_TYPE LastFMCover

#include <gio/gio.h>
#include <gel/gel-io.h>
#include <eina/iface.h>

#define random_bool(x) (g_random_int_range(0,100) >= x)

typedef enum {
	LASTFMCOVER_STATE_NONE,
	LASTFMCOVER_STATE_READY,
	LASTFMCOVER_STATE_FETCH_HTML,
	LASTFMCOVER_STATE_PARSE,
	LASTFMCOVER_STATE_FETCH_COVER,
	LASTFMCOVER_STATE_SUCCESS,
	LASTFMCOVER_STATE_ERROR
} LastFMCoverState;

typedef struct LastFMCover {
	EinaCover *cover;
	LomoStream *stream;
	GList *lookup_queue;
	GList *current_lookup;
	GCancellable *cancellable;
	LastFMCoverState state;
} LastFMCover;

typedef void (*LastFMCoverLookupFunc) (LastFMCover *self, LomoStream *stream);

static void
lastfmcover_lookup_finish(LastFMCover *self);

static void
lastfmcover_lookup_for_artist(LastFMCover *self, LomoStream *stream)
{
	gchar *uri;
	const gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	if (artist == NULL)
	{
		self->state = LASTFMCOVER_STATE_ERROR;
		lastfmcover_lookup_finish(self);
		return;
	}
	uri = g_strdup_printf("http://www.last.fm/music/%s", artist);
	gel_warn("Lookup artist: '%s'", uri);
	g_free(uri);
	self->state = LASTFMCOVER_STATE_ERROR;
	lastfmcover_lookup_finish(self);
}

static void
lastfmcover_lookup_for_album(LastFMCover *self, LomoStream *stream)
{
	gchar *uri;
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	if (!artist || !album)
	{
		self->state = LASTFMCOVER_STATE_ERROR;
		lastfmcover_lookup_finish(self);
		return;
	}
	uri = g_strdup_printf("http://www.last.fm/music/%s/%s", artist, album);
	gel_warn("Lookup album: '%s'", uri);
	g_free(uri);
	self->state = LASTFMCOVER_STATE_ERROR;
	lastfmcover_lookup_finish(self);
}

static void
lastfmcover_lookup_for_single(LastFMCover *self, LomoStream *stream)
{
	gchar *uri;
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	if (!artist || !title)
	{
		self->state = LASTFMCOVER_STATE_ERROR;
		lastfmcover_lookup_finish(self);
		return;
	}
	uri = g_strdup_printf("http://www.last.fm/music/%s/_/%s", artist, title);
	gel_warn("Lookup single: '%s'", uri);
	g_free(uri);
	self->state = LASTFMCOVER_STATE_ERROR;
	lastfmcover_lookup_finish(self);
}

static void
lastfmcover_setup_queue(LastFMCover *self, gchar *description)
{
	gint i;
	gchar **tmp = g_strsplit(description, ",", 0);

	g_list_free(self->lookup_queue);
	self->lookup_queue = NULL;

	for (i = 0; tmp[i] != NULL; i++)
	{
		LastFMCoverLookupFunc func = NULL;
		if (g_str_equal(tmp[i], "artist"))
			func = lastfmcover_lookup_for_artist;
		else if (g_str_equal(tmp[i], "album"))
			func = lastfmcover_lookup_for_album;
		else if (g_str_equal(tmp[i], "single"))
			func = lastfmcover_lookup_for_single;

		if (func != NULL)
			self->lookup_queue = g_list_prepend(self->lookup_queue, func);
	}
	g_strfreev(tmp);

	self->lookup_queue = g_list_reverse(self->lookup_queue);
}

static void
lastfmcover_lookup_finish(LastFMCover *self)
{
	LastFMCoverLookupFunc callback;
	switch (self->state)
	{
	case LASTFMCOVER_STATE_NONE:
		gel_warn("BUG?, state is NONE");
		// Bug?
		break;

	case LASTFMCOVER_STATE_READY:
		// Start lookups
		if (self->current_lookup == NULL)
		{
			// No more backends, reset and fail
			self->current_lookup = self->lookup_queue;
			eina_cover_backend_fail(self->cover);
			return;
		}

		callback = ((LastFMCoverLookupFunc) self->current_lookup->data);
		if (callback == NULL)
		{
			gel_warn("BUG: No valid lookup function");
			return;
		}
		callback(self, self->stream);
		break;

	case LASTFMCOVER_STATE_FETCH_HTML:
	case LASTFMCOVER_STATE_PARSE:
	case LASTFMCOVER_STATE_FETCH_COVER:
		gel_warn("BUG?, fetching or parsing");
		// Bugs?
		break;

	case LASTFMCOVER_STATE_SUCCESS:
		// Send cover to eina
		gel_warn("Got success!");
		break;

	case LASTFMCOVER_STATE_ERROR:
		// Next lookup or send fail to cover
		self->current_lookup = self->current_lookup->next;
		self->state = LASTFMCOVER_STATE_READY;
		lastfmcover_lookup_finish(self);
		break;

	default:
		break;
	}
}

void
lastfmcover_search(EinaCover *cover, LomoStream *stream, gpointer data)
{
	LastFMCover *self = (LastFMCover *) data;
	if (data == NULL)
	{
		gel_error("BUG: LastFMCover pointer is NULL");
		self->state = LASTFMCOVER_STATE_ERROR;
		eina_cover_backend_fail(cover);
		return;
	}

	if ((self->current_lookup = self->lookup_queue) == NULL)
	{
		gel_warn("BUG: No lookups availables");
		self->state = LASTFMCOVER_STATE_ERROR;
		eina_cover_backend_fail(cover);
		return;
	}

	if (stream == NULL)
	{
		gel_warn("BUG: No stream");
		self->state = LASTFMCOVER_STATE_ERROR;
		eina_cover_backend_fail(cover);
	}

	self->state = LASTFMCOVER_STATE_READY;
	self->stream = stream;
	g_object_ref(stream);
	lastfmcover_lookup_finish(self);
}

void
lastfmcover_cancel(EinaCover *cover, gpointer data)
{
	LastFMCover *self;
	if (data == NULL)
	{
		gel_error("BUG: plugin pointer is NULL");
		eina_cover_backend_fail(cover);
		return;
	}
	if ((self = EINA_PLUGIN_DATA(data)) == NULL)
	{
		gel_error("BUG: LastFMCover pointer is NULL");
		eina_cover_backend_fail(cover);
		return;
	}

	switch (self->state)
	{
	case LASTFMCOVER_STATE_SUCCESS:
	case LASTFMCOVER_STATE_ERROR:
	case LASTFMCOVER_STATE_PARSE:
		gel_warn("BUG: WTF #%d, parsing?? success?? fail??", self->state);
		// No break, go to safe state

	case LASTFMCOVER_STATE_NONE:
	case LASTFMCOVER_STATE_READY:
		self->current_lookup = self->lookup_queue;
		self->state = LASTFMCOVER_STATE_READY;
		break;

	case LASTFMCOVER_STATE_FETCH_HTML:
	case LASTFMCOVER_STATE_FETCH_COVER:
		if (!g_cancellable_is_cancelled(self->cancellable))
			g_cancellable_cancel(self->cancellable);
		g_cancellable_reset(self->cancellable);
		self->current_lookup = self->lookup_queue;
		self->state = LASTFMCOVER_STATE_READY;
		break;
	}
}

void
lastfmcover_search_wrapper(EinaCover *cover, LomoStream *stream, gpointer plugin)
{
	lastfmcover_search(cover, stream, EINA_PLUGIN_DATA(plugin));
}


void
lastfmcover_cancel_wrapper(EinaCover *cover, gpointer plugin)
{
	lastfmcover_cancel(cover, EINA_PLUGIN_DATA(plugin));
}

gboolean
lastfmcover_init(EinaPlugin *plugin, GError **error)
{
	gchar *order = "single,album,artist";
	
	// Create and setup LastFMCover data
	LastFMCover *self;
	self = g_new0(LastFMCover, 1);
	self->cover = eina_plugin_get_player_cover(plugin);
	self->cancellable = g_cancellable_new();
	self->state = LASTFMCOVER_STATE_NONE;
	lastfmcover_setup_queue(self, order);

	// Add backend to cover
	plugin->data = self;
	eina_cover_add_backend(self->cover, "lastfm",
		(EinaCoverBackendFunc) lastfmcover_search_wrapper, lastfmcover_cancel_wrapper,
		(gpointer) plugin);

	eina_iface_warn("Plugin lastfmcover initialized (%p %p)",
		plugin, plugin->data);
	return TRUE;
}

gboolean
lastfmcover_exit(EinaPlugin *plugin, GError **error)
{
	// Stop and free LastFMCover data
	LastFMCover *self = EINA_PLUGIN_DATA(plugin);

	lastfmcover_cancel(eina_plugin_get_player_cover(plugin), self);
	eina_cover_remove_backend(self->cover, "lastfm");
	g_object_unref(self->cancellable);
	g_list_free(self->lookup_queue);
	g_free(self);

	eina_iface_info("Plugin lastfmcover finalized");
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin lastfmcover_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("last.fm covers"),
	"0.0.0",
	N_("Fetch covers from last.fm"),
	N_(""),
	NULL,
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net",

	lastfmcover_init,
	lastfmcover_exit,

	NULL, NULL
};

