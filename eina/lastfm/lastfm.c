#define _POSIX_SOURCE

#include <eina/eina-plugin2.h>
#include <eina/application/application.h>
#include <eina/lomo/lomo.h>
#include "lastfm-priv.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

void
lastfm_plugin_build_preferences(GelPlugin *plugin);

static gboolean 
lastfm_submit_enable(EinaLastFM *self, GError **error);
static gboolean 
lastfm_submit_disable(EinaLastFM *self, GError **error);
static gboolean
lastfm_submit_write_config(EinaLastFM *self);
static void
lastfm_submit_submit(EinaLastFM *self);

static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaLastFM *self);

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastFM *self);

GEL_DEFINE_QUARK_FUNC(lastfm)

gboolean
lastfm_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaLastFM *data = g_new0(EinaLastFM, 1);
	gel_plugin_set_data(plugin, data);

	data->lomo = eina_plugin_get_lomo(plugin);
	lomo_player_hook_add(data->lomo, (LomoPlayerHook) lomo_hook_cb, data);

	GelUIApplication *app = eina_plugin_get_application(plugin);
	GSettings *settings = gel_ui_application_get_settings(app, LASTFM_PREFERENCES_DOMAIN);

	data->daemonpath = g_build_filename(gel_plugin_get_lib_dir(plugin), "lastfmsubmitd", "lastfmsubmitd", NULL);

	data->submit_enabled = g_settings_get_boolean(settings, LASTFM_SUBMIT_ENABLED_KEY);
	data->username = g_settings_get_string(settings, LASTFM_USERNAME_KEY);
	data->password = g_settings_get_string(settings, LASTFM_PASSWORD_KEY);

	lastfm_submit_write_config(data);
	lastfm_submit_enable(data, NULL);

	g_signal_connect(settings, "changed", (GCallback) settings_changed_cb, data);

	lastfm_plugin_build_preferences(plugin);
	return TRUE;
}

gboolean
lastfm_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaLastFM *data = gel_plugin_steal_data(plugin);
	lomo_player_hook_remove(data->lomo, (LomoPlayerHook) lomo_hook_cb);
	g_free(data->daemonpath);
	g_free(data);

	return TRUE;
}

void
lastfm_plugin_build_preferences(GelPlugin *plugin)
{
	EinaLastFM *self = (EinaLastFM *) gel_plugin_get_data(plugin);

	gchar *prefs_ui_path = g_build_filename(gel_plugin_get_data_dir(plugin), "preferences.ui", NULL);
	GtkWidget *prefs_ui = gel_ui_generic_new_from_file(prefs_ui_path);
	if (!prefs_ui)
	{
		g_warning(N_("Cannot create preferences UI"));
		g_free(prefs_ui_path);
		return;
	}
	g_free(prefs_ui_path);

	self->prefs_tab = eina_preferences_tab_new();
	g_object_set(self->prefs_tab,
		"widget", prefs_ui,
		"label-text", N_("Last FM"),
		NULL);

	GSettings *settings = g_settings_new(LASTFM_PREFERENCES_DOMAIN);
	eina_preferences_tab_bindv(self->prefs_tab,
		settings, LASTFM_SUBMIT_ENABLED_KEY,  LASTFM_SUBMIT_ENABLED_KEY,  "active",
		settings, LASTFM_USERNAME_KEY, LASTFM_USERNAME_KEY, "text",
		settings, LASTFM_PASSWORD_KEY, LASTFM_PASSWORD_KEY, "text",
		NULL);

	EinaPreferences *preferences = eina_plugin_get_preferences(plugin);
	eina_preferences_add_tab(preferences, self->prefs_tab);
	
}

// --
// Submit
// --
static gchar *
str_parser_cb(gchar key, LomoStream *stream)
{
	LomoTag tag = LOMO_TAG_INVALID;
	switch (key)
	{
	case 'a':
		tag = LOMO_TAG_ARTIST;
		break;
	case 't':
		tag = LOMO_TAG_TITLE;
		break;
	case 'b':
		tag = LOMO_TAG_ALBUM;
		break;
	default:
		return NULL;
	}
	gchar *ret = lomo_stream_get_tag(stream, tag);
	return ret ? g_strdup(ret) : NULL;
}

static gboolean
io_watch_cb(GIOChannel *io, GIOCondition cond, EinaLastFM *self)
{
	gchar *buff;
	gsize  size;

	if ((cond & G_IO_IN) || (cond & G_IO_PRI))
	{
		GError *error = NULL;
		GIOStatus status = g_io_channel_read_line(io, &buff, &size, NULL, &error);
		switch (status)
		{
		case G_IO_STATUS_NORMAL:
			if (size > 0)
			{
				buff[size-1] = '\0';
				if (io == self->io_err)
					g_warning("%s", buff);
				g_free(buff);
				return TRUE;
			}
			if (error)
			{
				g_warning("%s", error->message);
				g_error_free(error);
				lastfm_submit_disable(self, NULL);
				return FALSE;
			}
			return TRUE;
				
		default:
			lastfm_submit_disable(self, NULL);
			return FALSE;
		}
	}
	else if ((cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)))
	{
		lastfm_submit_disable(self, NULL);
		return FALSE;
	}
	else
	{
		lastfm_submit_disable(self, NULL);
		return FALSE;
	}
}

static gboolean
lastfm_submit_enable(EinaLastFM *self, GError **error)
{
	if (self->daemonpid != 0)
	{
		g_set_error(error, lastfm_quark(), EINA_LASTFM_ERROR_START_DAEMON,
			N_("There is a daemon running with PID: %d"), self->daemonpid);
		return FALSE;
	}

	// Folders
	gchar *spool = g_build_filename(eina_fs_get_cache_dir(), "lastfmsubmitd", "spool", NULL);

	if (!eina_fs_mkdir(spool, 0700, error))
	{
		g_warning(N_("Cannot create spool folder '%s': %s"), spool, (*error)->message);
		g_free(spool);
		return FALSE;
	}
	g_free(spool);

	gint outfd, errfd;
	GError *err = NULL;
	gchar *cmdl[] = { self->daemonpath, "--debug", "--no-daemon", NULL }; 
	if (!g_spawn_async_with_pipes(g_get_current_dir(), cmdl, NULL,
		G_SPAWN_SEARCH_PATH,
		NULL, NULL,
		&(self->daemonpid),
		NULL, &outfd, &errfd, &err))
	{
		g_set_error(error, lastfm_quark(), EINA_LASTFM_ERROR_START_DAEMON,
			N_("Cannot spawn daemon (%s): %s"), self->daemonpath, err->message);
		g_error_free(err);
		return FALSE;
	}
	else
	{
		g_warning("Daemon started as %d", self->daemonpid);
		self->io_out = g_io_channel_unix_new(outfd);
		self->io_err = g_io_channel_unix_new(errfd);
		g_io_channel_set_close_on_unref(self->io_out, TRUE);
		g_io_channel_set_close_on_unref(self->io_err, TRUE);
		self->out_id = g_io_add_watch(self->io_out, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
		self->err_id = g_io_add_watch(self->io_err, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
		return TRUE;
	}
}

static gboolean
lastfm_submit_disable(EinaLastFM *self, GError **error)
{
	if (!self->daemonpid)
		return TRUE;

	// Kill daemon
	if (self->daemonpid)
	{
		GPid pid = self->daemonpid;
		self->daemonpid = 0;
		kill(pid, 15);
		g_warning("Daemon stopped");
	}

	// Remove watchers
	if (self->out_id)
	{
		g_source_remove(self->out_id);
		self->out_id = 0;
	}
	if (self->err_id)
	{
		g_source_remove(self->err_id);
		self->err_id = 0;
	}

	// Close channles
	if (self->io_out)
	{
		g_io_channel_shutdown(self->io_out, FALSE, NULL);
		self->io_out = NULL;
	}
	if (self->io_err)
	{
		g_io_channel_shutdown(self->io_err, FALSE, NULL);
		self->io_err = NULL;
	}
	return TRUE;
}

static gboolean
lastfm_submit_write_config(EinaLastFM *self)
{
	if ((self->username == NULL) || (self->password == NULL))
		return FALSE;

	gchar *template =
		"[paths]\n"
		"spool=%s/%s/lastfmsubmitd/spool\n"
		"pidfile=%s/%s/lastfmsubmitd/pid\n"
		"log=%s/%s/lastfmsubmitd/log\n"
		"\n"
		"[account]\n"
		"user=%s\n"
		"password=%s\n";
	const gchar *cache   = g_get_user_cache_dir();

	gchar *dirname = g_build_filename(g_get_home_dir(), ".lastfmsubmitd", NULL);
	GError *err = NULL;
	if (!eina_fs_mkdir(dirname, 0700, &err))
	{
		g_warning("Cannot create conffile: %s", err->message);
		g_error_free(err);
		g_free(dirname);
		return FALSE;
	}
	
	gchar *pathname = g_build_filename(dirname, "conf", NULL);
	g_free(dirname);

	gchar *contents = g_strdup_printf(template,
		cache, PACKAGE,
		cache, PACKAGE,
		cache, PACKAGE,
		self->username, self->password);
	GError *error = NULL;
	if (!g_file_set_contents(pathname, contents, -1, &error))
	{
		g_warning("Cannot create conffile %s: %s", pathname, error->message);
		g_error_free(error);
		g_free(pathname);
		g_free(contents);
		return FALSE;
	}
	g_free(pathname);
	g_free(contents);
	return TRUE;
}

static void
lastfm_submit_submit(EinaLastFM *self)
{
	LomoStream *stream = lomo_player_get_current_stream(self->lomo);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	// Build YAML format
	gchar *tmp = gel_str_parser("---\n{album: %b\n}{title: %t\n}{artist: %a\n}", (GelStrParserFunc) str_parser_cb, stream);
	GString *str = g_string_new(tmp);
	g_free(tmp);

	g_string_append_printf(str, "length: %"G_GINT64_FORMAT":%02"G_GINT64_FORMAT"\n",
		lomo_nanosecs_to_secs(lomo_player_length_time(self->lomo)) / 60,
		lomo_nanosecs_to_secs(lomo_player_length_time(self->lomo)) % 60);

	struct tm *gmt;
	time_t     curtime = time (NULL);
	gmt = gmtime(&curtime);

	gchar strf[20];
	strftime(strf, 20, "%Y-%m-%d %H:%M:%S", gmt);
	g_string_append_printf(str, "time: !timestamp %s\n", strf);

	// Write to file
	gchar *spool = g_build_filename(g_get_user_cache_dir(), PACKAGE, "lastfmsubmitd", "spool", NULL);
	if (!eina_fs_mkdir(spool, 0700, NULL))
	{
		g_warning("Cannot create spooldir '%s'", spool);
		g_free(spool);
		g_string_free(str, TRUE);
		return;
	}

	gchar *spoolfile = g_build_filename(spool, "einalastfmXXXXXX", NULL);
	gint fd = g_mkstemp(spoolfile);
	g_free(spool);

	if ((fd == -1) || (write(fd, str->str, str->len) != strlen(str->str))) // dont use str->len, may be uses unicode count
	{
		g_warning(N_("Cannot write spoolfile '%s'"), spoolfile);
		g_free(spoolfile);
		return;
	}
	close(fd);
	g_string_free(str, TRUE);
	g_free(spoolfile);
}

static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaLastFM *self)
{
}

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastFM *self)
{
	if (ev.type != LOMO_PLAYER_HOOK_CHANGE)
		return FALSE;

	LomoStream *stream = lomo_player_get_current_stream(lomo);
	if (!stream)
		return FALSE;

	gint total  = lomo_nanosecs_to_secs(lomo_player_length_time(lomo));
	gint played = lomo_nanosecs_to_secs(lomo_player_stats_get_stream_time_played(lomo));
	g_warning("Got change. Played: %d:%02d (%d:%02d)", played / 60 , played % 60, total / 60 , total % 60);

	if (total < 30)
		return FALSE;
	if ((played < 240) && (played < (total/2)))
		return FALSE;

	// Submit!
	lastfm_submit_submit(self);
	g_warning("Submit to LastFM!");

	return FALSE;
}
