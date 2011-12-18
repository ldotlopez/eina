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

	LASTFM_SESSION *s;
	GQueue   *queue;
	GMutex   *queue_mutex, *sess_mutex;

	EinaPreferencesTab *prefs_tab;

	guint     reload_timeout_id;
	gboolean  logged;
	guint64   stamp;

} EinaCLastFMPluginPrivate;

EINA_PLUGIN_REGISTER(EINA_TYPE_CLASTFM_PLUGIN, EinaCLastFMPlugin, eina_clastfm_plugin)

void     clastfm_plugin_save_timestamp   (EinaCLastFMPlugin *self);
void     clastfm_plugin_submit_stream    (EinaCLastFMPlugin *self);
gboolean clastfm_plugin_run_commands     (EinaCLastFMPlugin *self);
void     clastfm_plugin_push_command     (EinaCLastFMPlugin *self, GVariant *packed_cmd);
gpointer clastfm_plugin_run_command_real (EinaCLastFMPlugin *self);
void     clastfm_plugin_schedule_reload  (EinaCLastFMPlugin *self);

gboolean reload_timeout_cb(EinaCLastFMPlugin *self);

gboolean
eina_clastfm_plugin_activate (EinaActivatable *plugin, EinaApplication *app, GError **error)
{
	EinaCLastFMPlugin *self = EINA_CLASTFM_PLUGIN(plugin);
	EinaCLastFMPluginPrivate *priv = self->priv;

	priv->queue = g_queue_new ();
	priv->queue_mutex = g_mutex_new ();
	priv->sess_mutex = g_mutex_new ();

	priv->lomo = eina_application_get_lomo (app);
	g_signal_connect_swapped ((GObject *) priv->lomo, "change",     (GCallback) clastfm_plugin_save_timestamp, self);
	g_signal_connect_swapped ((GObject *) priv->lomo, "eos",        (GCallback) clastfm_plugin_submit_stream,  self);
	g_signal_connect_swapped ((GObject *) priv->lomo, "pre-change", (GCallback) clastfm_plugin_submit_stream,  self);

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
	eina_application_remove_preferences_tab(app, priv->prefs_tab);
	if (priv->reload_timeout_id > 0)
	{
		g_source_remove(priv->reload_timeout_id);
		priv->reload_timeout_id = 0;
	}

	// Clear cmds queue
	g_mutex_lock(priv->queue_mutex);
	if (priv->queue)
	{
		g_queue_foreach (priv->queue, (GFunc) g_variant_unref, NULL);
		g_queue_free (priv->queue);
		priv->queue = NULL;
	}
	g_mutex_unlock(priv->queue_mutex);

	// Destroy session
	g_mutex_lock   (priv->sess_mutex);
	LASTFM_dinit   (priv->s);
	g_mutex_unlock (priv->sess_mutex);

	// Free resources
	gel_free_and_invalidate (priv->queue_mutex, NULL, g_mutex_free);
	gel_free_and_invalidate (priv->sess_mutex, NULL, g_mutex_free);

	priv->logged = FALSE;
	priv->stamp  = 0;

	return TRUE;
}

void
clastfm_plugin_save_timestamp (EinaCLastFMPlugin *self)
{
	g_return_if_fail (EINA_IS_CLASTFM_PLUGIN(self));

	struct timeval tv;
    gettimeofday (&tv, NULL);

	self->priv->stamp = (guint64) tv.tv_sec;
}

void
clastfm_plugin_schedule_reload(EinaCLastFMPlugin *self)
{
	g_return_if_fail(EINA_IS_CLASTFM_PLUGIN(self));
	EinaCLastFMPluginPrivate *priv = self->priv;

	if (priv->reload_timeout_id > 0)
		g_source_remove(priv->reload_timeout_id);
	priv->reload_timeout_id = g_timeout_add_seconds(5, (GSourceFunc) reload_timeout_cb, self);
}

void
clastfm_plugin_submit_stream (EinaCLastFMPlugin *self)
{
	g_return_if_fail (EINA_IS_CLASTFM_PLUGIN(self));

	EinaCLastFMPluginPrivate *priv = self->priv;

	LomoStream *stream = lomo_player_get_current_stream (priv->lomo);
	gint64 len    = LOMO_NANOSECS_TO_SECS (lomo_player_get_length (priv->lomo));
	gint64 played = LOMO_NANOSECS_TO_SECS (lomo_player_stats_get_stream_time_played (priv->lomo));

	if ((len < 30) || (played < (len/2)))
	{
		debug ("Stream hasn't been played enough");
		return;
	}

	GVariant *cmd = g_variant_new ("(sssstt)",
		"track_scrobble",
		g_value_get_string (lomo_stream_get_tag (stream, "title")),
		g_value_get_string (lomo_stream_get_tag (stream, "album")),
		g_value_get_string (lomo_stream_get_tag (stream, "artist")),
		(guint64) priv->stamp,
		(guint64) LOMO_NANOSECS_TO_SECS(lomo_player_get_length (priv->lomo)));
	clastfm_plugin_push_command (self, cmd);
}

gboolean
clastfm_plugin_run_commands (EinaCLastFMPlugin *self)
{
	g_thread_create ((GThreadFunc) clastfm_plugin_run_command_real, self, FALSE, NULL);
	return FALSE;
}

void
clastfm_plugin_push_command (EinaCLastFMPlugin *self, GVariant *packed_cmd)
{
	g_return_if_fail (EINA_IS_CLASTFM_PLUGIN(self));
	EinaCLastFMPluginPrivate *priv = self->priv;

	g_mutex_lock      (priv->queue_mutex);
	g_queue_push_tail (priv->queue, packed_cmd);
	g_mutex_unlock    (priv->queue_mutex);

	clastfm_plugin_run_commands (self);
}

gpointer
clastfm_plugin_run_command_real (EinaCLastFMPlugin *self)
{
	g_return_val_if_fail (EINA_IS_CLASTFM_PLUGIN(self), FALSE);
	EinaCLastFMPluginPrivate *priv = self->priv;

	if (!g_mutex_trylock (priv->queue_mutex))
	{
		g_timeout_add (500, (GSourceFunc) clastfm_plugin_run_commands, self);
		g_thread_exit (0);
		return NULL;
	}
	GVariant *packed_cmd = g_queue_pop_head (priv->queue);
	g_mutex_unlock (priv->queue_mutex);

	GVariant *cmd_ = g_variant_get_child_value (packed_cmd, 0);
	const gchar *cmd = g_variant_get_string (cmd_, NULL);

	debug("Run command %s", cmd);

	/* All commands but dinit and login need a valid session and loggedin */
	if (!(g_str_equal("dinit", cmd) || g_str_equal("login", cmd)))
		g_return_val_if_fail(priv->s && priv->logged, NULL);

	// Auto create LASTFM session
	if (!priv->s)
	{
		g_mutex_lock (priv->sess_mutex);
		priv->s = LASTFM_init(API_KEY, API_SECRET);
		g_mutex_unlock (priv->sess_mutex);
	}

	if (g_str_equal("dinit", cmd))
	{
		if (priv->s)
		{
			g_mutex_lock (priv->sess_mutex);
			LASTFM_dinit(priv->s);
			priv->s = NULL;
			g_mutex_unlock (priv->sess_mutex);
		}
	}


	/*
	 * login
	 */
	else if (g_str_equal ("login", cmd))
	{
		GVariant *v[2] = {
			g_variant_get_child_value (packed_cmd, 1),
			g_variant_get_child_value (packed_cmd, 2)
		};
		const gchar *user = g_variant_get_string (v[0], NULL);
		const gchar *pass = g_variant_get_string (v[1], NULL);

		g_mutex_lock (priv->sess_mutex);
		gint rv = LASTFM_login (priv->s, user, pass);
		priv->logged = (rv == 0);
		debug ("Login returned=%d", rv);
		debug ("      status=%s", LASTFM_status (priv->s));
		g_mutex_unlock (priv->sess_mutex);

		g_variant_unref (v[0]);
		g_variant_unref (v[1]);
	}

	/*
	 * scrobble
	 */
	else if (g_str_equal ("track_scrobble", cmd))
	{
		g_return_val_if_fail (priv->logged, NULL);

		GVariant *v[5] = { NULL };
		for (guint i = 0; i < G_N_ELEMENTS(v); i++)
			v[i] = g_variant_get_child_value (packed_cmd, i+1);

 		g_mutex_lock (priv->sess_mutex);
		LASTFM_track_scrobble (priv->s,       // session
			(char *) g_variant_get_string (v[0], NULL), // title          (char*)
			(char *) g_variant_get_string (v[1], NULL), // album          (char *)
			(char *) g_variant_get_string (v[2], NULL), // artist         (char *)
			g_variant_get_uint64(v[3]),                // start time     (time_t)
			g_variant_get_uint64(v[4]),                // length in secs (unsigned int)
			0,                                         // track no       (unsigned int)
			0,                                         // md ID          (unsigned int)
			NULL);                                     // result         (LFMList **)
		debug ("status=%s\n", LASTFM_status (priv->s));
		g_mutex_unlock (priv->sess_mutex);

		for (guint i = 0; i < G_N_ELEMENTS(v); i++)
			g_variant_unref (v[i]);
	}

	/*
	 * album_get_info
	 */
	else if (g_str_equal ("-album_get_info", cmd))
	{
		GVariant *v[2] = {
			g_variant_get_child_value (packed_cmd, 1),
			g_variant_get_child_value (packed_cmd, 2)
		};
		const gchar *artist = g_variant_get_string (v[0], NULL);
		const gchar *album  = g_variant_get_string (v[1], NULL);

		g_warn_if_fail (artist && album);
		if (artist && album)
		{
			g_mutex_lock (priv->sess_mutex);
			LASTFM_ALBUM_INFO *album_info = LASTFM_album_get_info (priv->s, artist, album);
			g_mutex_unlock (priv->sess_mutex);
			LASTFM_print_album_info (stdout,album_info);
			LASTFM_free_album_info (album_info);
		}

		g_variant_unref (v[0]);
		g_variant_unref (v[1]);
	}

	else
	{
		g_warning ("Unknow cmd %s", cmd);
	}

	g_variant_unref (cmd_);

	g_mutex_unlock (priv->queue_mutex);
	g_thread_exit (0);

	return NULL;
}

gboolean
reload_timeout_cb(EinaCLastFMPlugin *self)
{
	g_return_val_if_fail(EINA_IS_CLASTFM_PLUGIN(self), FALSE);
	EinaCLastFMPluginPrivate *priv = self->priv;

	priv->reload_timeout_id = 0;
	gboolean enabled = g_settings_get_boolean(priv->settings, CLASTFM_SUBMIT_ENABLED_KEY);
	if (!enabled)
	{
		clastfm_plugin_push_command(self, g_variant_new("(s)", "dinit"));
	}

	else
	{
		gchar *d[2] = {
			g_settings_get_string(priv->settings,  CLASTFM_USERNAME_KEY),
			g_settings_get_string(priv->settings,  CLASTFM_PASSWORD_KEY)
		};
		clastfm_plugin_push_command(self, g_variant_new("(s)", "dinit"));
		clastfm_plugin_push_command(self, g_variant_new("(sss)", "login", d[0], d[1]));
	}

	return FALSE;
}


