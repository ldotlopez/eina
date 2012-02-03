#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/time.h>
#include <time.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-extension.h>
#include <eina/lomo/eina-lomo-plugin.h>
#include <eina/preferences/eina-preferences-plugin.h>
#include <glib/gstdio.h>
#include <clastfm.h>
#include "lastfm-thread.h"

#define DEBUG 1
#define DEBUG_PREFIX "EinaCLastFMPlugin "
#if DEBUG
#	define debug(...) g_debug (DEBUG_PREFIX __VA_ARGS__)
#else
#	define debug(...) ;
#endif

#define EINA_TYPE_CLASTFM_PLUGIN eina_clastfm_plugin_get_type ()
#define EINA_CLASTFM_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_CLASTFM_PLUGIN, EinaCLastFMPlugin))
#define EINA_CLASTFM_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_CLASTFM_PLUGIN, EinaCLastFMPluginClass))
#define EINA_IS_CLASTFM_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_CLASTFM_PLUGIN))
#define EINA_IS_CLASTFM_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_CLASTFM_PLUGIN))
#define EINA_CLASTFM_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_CLASTFM_PLUGIN, EinaCLastFMPluginClass))

#define API_KEY    "2c8ac587be681fcfe69e226690414ad8"
#define API_SECRET "b7318c221263c4b1faccbf617a19b329"
#define CLASTFM_PREFERENCES_DOMAIN EINA_DOMAIN".preferences.lastfm"
#define CLASTFM_SUBMIT_ENABLED_KEY "submit-enabled"
#define CLASTFM_USERNAME_KEY       "username"
#define CLASTFM_PASSWORD_KEY       "password"

typedef struct {
	LomoPlayer *lomo;
	GSettings *settings;
	time_t stamp;

	EinaPreferencesTab *prefs_tab;

	guint     reload_timeout_id;

	LastFMThread *th;
} EinaCLastFMPluginPrivate;

EINA_PLUGIN_REGISTER(EINA_TYPE_CLASTFM_PLUGIN, EinaCLastFMPlugin, eina_clastfm_plugin)

void     clastfm_plugin_save_timestamp   (EinaCLastFMPlugin *self);
gboolean clastfm_plugin_submit_stream    (EinaCLastFMPlugin *self);
void     clastfm_plugin_schedule_reload  (EinaCLastFMPlugin *self);
gboolean reload_timeout_cb(EinaCLastFMPlugin *self);

gboolean
eina_clastfm_plugin_activate (EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaCLastFMPlugin *self = EINA_CLASTFM_PLUGIN(plugin);
	EinaCLastFMPluginPrivate *priv = self->priv;

	clastfm_plugin_save_timestamp(self);

	priv->lomo = eina_application_get_lomo (app);
	g_signal_connect_swapped ((GObject *) priv->lomo, "change",     (GCallback) clastfm_plugin_save_timestamp, self);
	g_signal_connect_swapped ((GObject *) priv->lomo, "eos",        (GCallback) clastfm_plugin_submit_stream,  self);
	g_signal_connect_swapped ((GObject *) priv->lomo, "pre-change", (GCallback) clastfm_plugin_submit_stream,  self);

	priv->th = lastfm_thread_new();

	gchar *datadir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE(plugin));
	gchar *prefs_ui_path = g_build_filename (datadir, "preferences.ui", NULL);
	g_free (datadir);

	GtkWidget *widget = gel_ui_generic_new_from_file (prefs_ui_path);
	g_free (prefs_ui_path);

	if (!widget)
		g_warning("Unable to build preferences dialog");
	else
	{
		priv->prefs_tab = eina_preferences_tab_new();
		g_object_set(priv->prefs_tab,
			"widget",      widget,
			"label-text",  "Last FM",
			NULL);

		priv->settings = eina_application_get_settings(app, CLASTFM_PREFERENCES_DOMAIN);
		eina_preferences_tab_bindv(priv->prefs_tab,
			priv->settings, CLASTFM_SUBMIT_ENABLED_KEY, CLASTFM_SUBMIT_ENABLED_KEY,  "active",
			priv->settings, CLASTFM_USERNAME_KEY, CLASTFM_USERNAME_KEY, "text",
			priv->settings, CLASTFM_PASSWORD_KEY, CLASTFM_PASSWORD_KEY, "text",
			NULL);
		g_signal_connect_swapped(priv->settings, "changed", (GCallback) clastfm_plugin_schedule_reload, plugin);

		eina_application_add_preferences_tab(app, priv->prefs_tab);
		clastfm_plugin_schedule_reload(self);
	}

	return TRUE;
}

gboolean
eina_clastfm_plugin_deactivate (EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaCLastFMPlugin *self = EINA_CLASTFM_PLUGIN(plugin);
	EinaCLastFMPluginPrivate *priv = self->priv;

	// Remove UI
	if (priv->reload_timeout_id > 0)
	{
		g_source_remove(priv->reload_timeout_id);
		priv->reload_timeout_id = 0;
	}
	eina_application_remove_preferences_tab(app, priv->prefs_tab);

	gel_object_free_and_invalidate(priv->th);

	return TRUE;
}

/**
 * clastfm_plugin_save_timestamp:
 * @self: An #EinaCLastFMPlugin
 *
 * Saves current timestamp into @self
 */
void
clastfm_plugin_save_timestamp (EinaCLastFMPlugin *self)
{
	g_return_if_fail (EINA_IS_CLASTFM_PLUGIN(self));

	struct timeval tv;
	gettimeofday (&tv, NULL);

	self->priv->stamp = tv.tv_sec;
}

/**
 * clastfm_plugin_schedule_reload:
 * @self: An #EinaCLastFMPlugin
 *
 * Schedules @self to reload settings. If another reques has been request but
 * hasn't been run it's rescheduled.
 */
void
clastfm_plugin_schedule_reload(EinaCLastFMPlugin *self)
{
	g_return_if_fail(EINA_IS_CLASTFM_PLUGIN(self));
	EinaCLastFMPluginPrivate *priv = self->priv;

	if (priv->reload_timeout_id > 0)
		g_source_remove(priv->reload_timeout_id);
	priv->reload_timeout_id = g_timeout_add_seconds(5, (GSourceFunc) reload_timeout_cb, self);
}

/**
 * clastfm_plugin_submit_stream:
 * @self: An #EinaCLastFMPlugin
 *
 * Submits or not current stream from internal #LomoPlayer if it matches
 * some conditions
 *
 * Returns: Whatever or not stream is submited
 */
gboolean
clastfm_plugin_submit_stream (EinaCLastFMPlugin *self)
{
	g_return_val_if_fail (EINA_IS_CLASTFM_PLUGIN(self), FALSE);
	EinaCLastFMPluginPrivate *priv = self->priv;

	LomoStream *stream = lomo_player_get_current_stream (priv->lomo);
	g_return_val_if_fail(LOMO_IS_STREAM(stream), FALSE);

	gint64 len    = LOMO_NANOSECS_TO_SECS (lomo_player_get_length (priv->lomo));
	gint64 played = LOMO_NANOSECS_TO_SECS (lomo_player_stats_get_stream_time_played (priv->lomo));

	if ((len < 30) || (played < (len/2)))
	{
		debug ("Stream hasn't been played enough");
		return FALSE;
	}

	LastFMThreadMethodCall call = { .method_name = "track_scrobble",
		.title  = (gchar *) g_value_get_string (lomo_stream_get_tag (stream, "title")),
		.album  = (gchar *) g_value_get_string (lomo_stream_get_tag (stream, "album")),
		.artist = (gchar *) g_value_get_string (lomo_stream_get_tag (stream, "artist")),
		.start_stamp = (guint64) priv->stamp,
		.length = (guint64) LOMO_NANOSECS_TO_SECS(lomo_player_get_length (priv->lomo)) };

	lastfm_thread_call(priv->th, &call);
	return TRUE;
}

void
login_cb(gpointer data)
{
	g_debug("Login was finished. I'm in thread %p . Data:%d", g_thread_self(), GPOINTER_TO_INT(data));
}


/**
 * reload_timeout_cb: (skip):
 * @self: An #EinaCLastFMPlugin
 *
 * Reloads settings from #GSettings. This function is meant to be runned as a
 * #GSourceFunc
 *
 * Returns: %FALSE
 */
gboolean
reload_timeout_cb(EinaCLastFMPlugin *self)
{
	g_return_val_if_fail(EINA_IS_CLASTFM_PLUGIN(self), FALSE);
	EinaCLastFMPluginPrivate *priv = self->priv;

	priv->reload_timeout_id = 0;
	gboolean enabled = g_settings_get_boolean(priv->settings, CLASTFM_SUBMIT_ENABLED_KEY);
	if (!enabled)
	{
		LastFMThreadMethodCall dinit = { .method_name = "dinit" };
		lastfm_thread_call(priv->th, &dinit);
	}

	else
	{
		g_debug("Login has begin. I'm in thread %p", g_thread_self());
		LastFMThreadMethodCall
			dinit = { .method_name = "dinit" },
			init  = { .method_name = "init", .api_key = API_KEY, .api_secret = API_SECRET },
			login = { .method_name = "login",
				.username = g_settings_get_string(priv->settings,  CLASTFM_USERNAME_KEY),
				.password = g_settings_get_string(priv->settings,  CLASTFM_PASSWORD_KEY) };
		lastfm_thread_call(priv->th, &dinit);
		lastfm_thread_call(priv->th, &init);
		lastfm_thread_call_full(priv->th, &login, (GCallback) login_cb, GINT_TO_POINTER(42), NULL);
	}

	return FALSE;
}

