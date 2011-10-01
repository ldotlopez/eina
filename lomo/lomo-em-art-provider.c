#include "lomo-em-art-provider.h"
#include "lomo-em-art.h"
#include "lomo-em-art-backends.h"
#include <gel/gel.h>
#include <glib/gi18n.h>

#define DEBUG 0
#define DEBUG_PREFIX "LomoEMArtProvider"
#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX " " __VA_ARGS__)
#else
#	define debug(...) ;
#endif

G_DEFINE_TYPE (LomoEMArtProvider, lomo_em_art_provider, G_TYPE_OBJECT)

struct _LomoEMArtProviderPrivate {
	LomoPlayer *player;
	LomoEMArt  *art;
};

enum {
	DEFAULT_COVER_PATH,
	LOADING_COVER_PATH,
	DEFAULT_COVER_URI,
	LOADING_COVER_URI,

	COVER_N_STRINGS
};
static gchar *cover_strings[COVER_N_STRINGS] = { NULL };

static void init_paths(void);

static void lomo_weak_ref_cb(LomoEMArtProvider *self, LomoPlayer *lomo);

static void art_search_cb(LomoEMArtSearch *search, gpointer data);
static void change_cb    (LomoPlayer *lomo, gint from, gint to, LomoEMArtProvider *self);
static void all_tags_cb  (LomoPlayer *lomo, LomoStream *stream, LomoEMArtProvider *self);

static void
lomo_em_art_provider_dispose (GObject *object)
{
	LomoEMArtProvider *self = LOMO_EM_ART_PROVIDER(object);
	LomoEMArtProviderPrivate *priv = self->priv;

	lomo_em_art_provider_set_player(self, NULL);

	if (priv->art)
	{
		g_object_unref(priv->art);
		priv->art = NULL;
	}

	G_OBJECT_CLASS (lomo_em_art_provider_parent_class)->dispose (object);
}

static void
lomo_em_art_provider_class_init (LomoEMArtProviderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoEMArtProviderPrivate));

	object_class->dispose = lomo_em_art_provider_dispose;
}

static void
lomo_em_art_provider_init (LomoEMArtProvider *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_EM_ART_PROVIDER, LomoEMArtProviderPrivate));
	self->priv->art = lomo_em_art_new();

	LomoEMArtClass *art_class = LOMO_EM_ART_CLASS(G_OBJECT_GET_CLASS(self->priv->art));
	lomo_em_art_class_add_backend(art_class, "infolder",
		lomo_em_art_infolder_sync_backend_search, NULL,
		NULL, NULL);
}

LomoEMArtProvider*
lomo_em_art_provider_new (void)
{
	static LomoEMArtProvider* self = NULL;
	if (self)
		return g_object_ref(self);
	self = g_object_new (LOMO_TYPE_EM_ART_PROVIDER, NULL);
	return self;
}

void
lomo_em_art_provider_set_player(LomoEMArtProvider *self, LomoPlayer *lomo)
{
	g_return_if_fail(LOMO_IS_EM_ART_PROVIDER(self));
	if (lomo)
		g_return_if_fail(LOMO_IS_PLAYER(lomo));

	LomoEMArtProviderPrivate *priv = self->priv;

	// Unlink previous lomo player
	if (priv->player)
	{
		g_signal_handlers_disconnect_by_func(priv->player, change_cb,   self);
		g_signal_handlers_disconnect_by_func(priv->player, all_tags_cb, self);
		g_object_weak_unref((GObject *) lomo, (GWeakNotify) lomo_weak_ref_cb, self);
	}

	// Setup new lomo player
	if (lomo)
	{
		g_object_weak_ref((GObject *) lomo, (GWeakNotify) lomo_weak_ref_cb, self);
		g_signal_connect(lomo, "change",   (GCallback) change_cb,   self);
		g_signal_connect(lomo, "all-tags", (GCallback) all_tags_cb, self);
	}
}

static void init_paths(void)
{
	static gboolean _init = FALSE;
	if (_init)
		return;

	_init = TRUE;

	cover_strings[DEFAULT_COVER_PATH] = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "cover-default.png");
	cover_strings[LOADING_COVER_PATH] = gel_resource_locate(GEL_RESOURCE_TYPE_IMAGE, "cover-loading.png");

	if (!cover_strings[DEFAULT_COVER_PATH])
		g_warning(_("Unable to locate '%s'"), "cover-default.png");
	else
		cover_strings[DEFAULT_COVER_URI] = g_filename_to_uri(cover_strings[DEFAULT_COVER_PATH], NULL, NULL);

	if (!cover_strings[LOADING_COVER_PATH])
		g_warning(_("Unable to locate '%s'"), "cover-loading.png");
	else
		cover_strings[LOADING_COVER_URI] = g_filename_to_uri(cover_strings[LOADING_COVER_PATH], NULL, NULL);
}

/**
 * lomo_em_art_provider_get_default_cover_path:
 *
 * Get the path for the default cover art.
 *
 * Returns: The path
 */
const gchar *
lomo_em_art_provider_get_default_cover_path()
{
	init_paths();
	return cover_strings[DEFAULT_COVER_PATH];
}

/**
 * lomo_em_art_provider_get_default_cover_uri:
 *
 * Get the URI for the default cover art.
 *
 * Returns: The URI
 */
const gchar *
lomo_em_art_provider_get_default_cover_uri()
{
	init_paths();
	return cover_strings[DEFAULT_COVER_URI];
}

/**
 * lomo_em_art_provider_get_loading_cover_path:
 *
 * Get the path for the loading cover art
 *
 * Returns: The path
 */
const gchar *
lomo_em_art_provider_get_loading_cover_path()
{
	init_paths();
	return cover_strings[LOADING_COVER_PATH];
}

/**
 * lomo_em_art_provider_get_loading_cover_uri:
 *
 * Get the URI for the loading cover URI.
 *
 * Returns: The URI
 */
const gchar *
lomo_em_art_provider_get_loading_cover_uri()
{
	init_paths();
	return cover_strings[LOADING_COVER_URI];
}

/**
 * lomo_em_art_provider_init_stream:
 * @self: An #LomoEMArtProvider
 * @stream: (transfer none): A #LomoStream to init
 *
 * Search art data for @stream. @stream while emit the
 * LomoStream:extended-metadata-updated signal when finished
 */
void
lomo_em_art_provider_init_stream(LomoEMArtProvider *self, LomoStream *stream)
{
	g_return_if_fail(LOMO_IS_EM_ART_PROVIDER(self));
	g_return_if_fail(LOMO_IS_STREAM(stream));

	gpointer t = g_object_get_data((GObject *) stream, "art-uri-searched");
	if (t != NULL)
	{
		debug("Stream already initialized, return");
		return;
	}

	const gchar *art_uri = lomo_stream_get_extended_metadata_as_string(stream, "art-uri");
	debug("Init stream %p with art-uri: %s", stream, art_uri);
	if (art_uri)
	{
		if (g_str_equal(art_uri, lomo_em_art_provider_get_loading_cover_uri()))
		{
			debug("Stream %p already has art-uri and it's loading cover.", stream);
			return;
		}
		if (!g_str_equal(art_uri, lomo_em_art_provider_get_default_cover_uri()))
		{
			debug("Stream %p has custom art-uri: %s", stream, art_uri);
			return;
		}
	}

	art_uri = NULL;
	if (lomo_stream_get_all_tags_flag(stream))
	{
		LomoEMArtSearch *search = lomo_em_art_search(self->priv->art, stream, art_search_cb, NULL);
		debug("Stream %p has all_tags flag, search started %p", stream, search);
		if (search)
			art_uri = (gchar *) lomo_em_art_provider_get_loading_cover_uri();
	}
	else
	{
		debug("Stream %p doesnt have all_tags flags, wait for it. default cover is set", stream);
		art_uri = (gchar *) lomo_em_art_provider_get_default_cover_uri();
	}

	if (art_uri)
	{
		lomo_stream_set_extended_metadata_as_string(stream, "art-uri", art_uri);
		// g_object_set_data((GObject *) stream, "art-uri-searched", (gpointer) TRUE);
	}
}



static void
lomo_weak_ref_cb(LomoEMArtProvider *self, LomoPlayer *old_player)
{
	LomoEMArtProviderPrivate *priv = self->priv;
	priv->player = NULL;
}

/*
 * Callbacks
 */
static void
art_search_cb(LomoEMArtSearch *search, gpointer data)
{
	const gchar *res = lomo_em_art_search_get_result(search);
	LomoStream *stream = lomo_em_art_search_get_stream(search);
	#if DEBUG
	debug ("Got result for stream %p: %p", stream, res);
	#endif

	lomo_stream_set_extended_metadata_as_string(lomo_em_art_search_get_stream(search),
		"art-uri",
		res ? res : cover_strings[DEFAULT_COVER_URI]);

	g_object_set_data((GObject *) stream, "art-uri-searched", (gpointer) TRUE);

	debug("Extended metadata set to: %s",
		lomo_stream_get_extended_metadata_as_string(lomo_em_art_search_get_stream(search), "art-uri"));
}

static void
change_cb(LomoPlayer *lomo, gint from, gint to, LomoEMArtProvider *self)
{
	g_return_if_fail(LOMO_IS_PLAYER(lomo));
	g_return_if_fail(LOMO_IS_EM_ART_PROVIDER(self));

	if (to == -1)
		return;

	LomoStream *streams[2] = {
		lomo_player_get_nth_stream(lomo, to),
		lomo_player_get_nth_stream(lomo, lomo_player_get_next(lomo)) };

	for (guint i = 0; i < G_N_ELEMENTS(streams); i++)
	{
		if (!streams[i])
			continue;

		lomo_em_art_provider_init_stream(self, streams[i]);
	}
}

static void
all_tags_cb(LomoPlayer *lomo, LomoStream *stream, LomoEMArtProvider *self)
{
	const gchar *stream_art_uri = lomo_stream_get_extended_metadata_as_string(stream, "art-uri");
	if (g_strcmp0(stream_art_uri, lomo_em_art_provider_get_default_cover_uri()) == 0)
	{
		debug("Catch a stream all-tags signal for stream with default cover");
		lomo_em_art_provider_init_stream(self, stream);
	}
}

