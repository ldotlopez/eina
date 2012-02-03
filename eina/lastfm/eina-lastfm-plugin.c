/*
 * eina/lastfm/eina-lastfm-plugin.c
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

#define _POSIX_SOURCE

#include "lastfm-priv.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-fs.h>
#include <eina/ext/eina-extension.h>
#include <eina/lomo/eina-lomo-plugin.h>

#define EINA_TYPE_LASTFM_PLUGIN         (eina_lastfm_plugin_get_type ())
#define EINA_LASTFM_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_LASTFM_PLUGIN, EinaLastfmPlugin))
#define EINA_LASTFM_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_LASTFM_PLUGIN, EinaLastfmPlugin))
#define EINA_IS_LASTFM_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_LASTFM_PLUGIN))
#define EINA_IS_LASTFM_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_LASTFM_PLUGIN))
#define EINA_LASTFM_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_LASTFM_PLUGIN, EinaLastfmPluginClass))
EINA_PLUGIN_REGISTER(EINA_TYPE_LASTFM_PLUGIN, EinaLastfmPlugin, eina_lastfm_plugin)

#define SCHED_TIMEOUT 5
#define DEBUG_DAEMON  0

static void     lastfm_plugin_build_preferences(EinaLastfmPlugin *plugin);
static gboolean lastfm_submit_enable      (EinaLastfmPlugin *plugin);
static gboolean lastfm_submit_disable     (EinaLastfmPlugin *plugin);
static void     lastfm_submit_submit      (EinaLastfmPlugin *plugin);
static gboolean lastfm_submit_write_config(EinaLastfmPlugin *plugin);

static void     lastfm_submit_sched_write_config  (EinaLastfmPlugin *plugin);
static gboolean lastfm_submit_write_config_wrapper(EinaLastfmPlugin *plugin);

static gchar *  str_parser_cb(gchar key, LomoStream *stream);
#if DEBUG_DAEMON
static gboolean io_watch_cb(GIOChannel *io, GIOCondition cond, EinaLastfmPlugin *plugin);
#endif

static void settings_changed_cb(GSettings *settings, const gchar *key, EinaLastfmPlugin *plugin);

static gboolean lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastfmPlugin *plugin);

static gboolean
eina_lastfm_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaLastfmPlugin      *plugin = EINA_LASTFM_PLUGIN(activatable);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	priv->lomo = eina_application_get_lomo(app);
	lomo_player_hook_add(priv->lomo, (LomoPlayerHook) lomo_hook_cb, plugin);

	priv->settings  = eina_application_get_settings(app, LASTFM_PREFERENCES_DOMAIN);

	gchar *datadir = peas_extension_base_get_data_dir(PEAS_EXTENSION_BASE(activatable));
	priv->daemonpath = g_build_filename(datadir, "lastfmsubmitd", "lastfmsubmitd", NULL);
	g_free(datadir);

	lastfm_plugin_build_preferences(plugin);

	lastfm_submit_write_config(plugin);
	lastfm_submit_enable(plugin);

	return TRUE;
}

static gboolean
eina_lastfm_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaLastfmPlugin      *plugin = EINA_LASTFM_PLUGIN(activatable);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	lastfm_submit_disable(plugin);
	eina_application_remove_preferences_tab(app, priv->prefs_tab);
	lomo_player_hook_remove(priv->lomo, (LomoPlayerHook) lomo_hook_cb);

	g_free(priv->daemonpath);

	return TRUE;
}

static void
lastfm_plugin_build_preferences(EinaLastfmPlugin *plugin)
{
	g_return_if_fail(EINA_IS_LASTFM_PLUGIN(plugin));
	EinaLastfmPluginPrivate *priv = plugin->priv;

	gchar *datadir = peas_extension_base_get_data_dir(PEAS_EXTENSION_BASE(plugin));
	gchar *prefs_ui_path = g_build_filename(datadir, "preferences.ui", NULL);
	g_free(datadir);

	GtkWidget *prefs_ui = gel_ui_generic_new_from_file(prefs_ui_path);
	g_free(prefs_ui_path);
	if (!prefs_ui)
	{
		g_warning(N_("Cannot create preferences UI"));
		return;
	}

	gchar *icon_filename = g_build_filename(datadir, "lastfm.png", NULL);
	GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(icon_filename, 16, 16, TRUE, NULL);
	gel_free_and_invalidate(icon_filename, NULL, g_free);

	priv->prefs_tab = eina_preferences_tab_new();
	g_object_set(priv->prefs_tab,
		"widget",      prefs_ui,
		"label-text",  N_("Last FM"),
		"label-image", gtk_image_new_from_pixbuf(pb),
		NULL);
	gel_free_and_invalidate(pb, NULL, g_object_unref);

	eina_preferences_tab_bindv(priv->prefs_tab,
		priv->settings, LASTFM_SUBMIT_ENABLED_KEY, LASTFM_SUBMIT_ENABLED_KEY,  "active",
		priv->settings, LASTFM_USERNAME_KEY, LASTFM_USERNAME_KEY, "text",
		priv->settings, LASTFM_PASSWORD_KEY, LASTFM_PASSWORD_KEY, "text",
		NULL);
	g_signal_connect(priv->settings, "changed", (GCallback) settings_changed_cb, plugin);

	EinaApplication *app = eina_activatable_get_application(EINA_ACTIVATABLE(plugin));
	eina_application_add_preferences_tab(app, priv->prefs_tab);
}


// **********
// * Submit *
// **********

// --
// Start daemon
// --
static gboolean
lastfm_submit_enable(EinaLastfmPlugin *plugin)
{
	g_return_val_if_fail(EINA_IS_LASTFM_PLUGIN(plugin), FALSE);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	if (priv->daemonpid)
		return TRUE;

	// Create folders
	GError *error = NULL;
	gchar *spool = g_build_filename(eina_fs_get_cache_dir(), "lastfmsubmitd", "spool", NULL);
	if (!eina_fs_mkdir(spool, 0700, &error))
	{
		g_warning(N_("Cannot create spool folder '%s': %s"), spool, error->message);
		g_error_free(error);
		g_free(spool);
		return FALSE;
	}
	g_free(spool);

	// Launch and watch daemon
	gint outfd, errfd;
	GError *err = NULL;
	gchar *cmdl[] = { "python", priv->daemonpath, "--debug", "--no-daemon", NULL }; 
	if (!
		#if DEBUG_DAEMON
		g_spawn_async_with_pipes
		#else
		g_spawn_async_with_pipes
		#endif
		(g_get_current_dir(), cmdl, NULL,
		G_SPAWN_SEARCH_PATH,
		NULL, NULL,
		&(priv->daemonpid),
		NULL, &outfd, &errfd,
		&err))
	{
		g_warning(N_("Cannot spawn daemon (%s): %s"), priv->daemonpath, err->message);
		g_error_free(err);
		return FALSE;
	}
	else
	{
		#if DEBUG_DAEMON
		g_warning("Daemon started as %d", priv->daemonpid);
		priv->io_out = g_io_channel_unix_new(outfd);
		priv->io_err = g_io_channel_unix_new(errfd);
		g_io_channel_set_close_on_unref(priv->io_out, TRUE);
		g_io_channel_set_close_on_unref(priv->io_err, TRUE);
		priv->out_id = g_io_add_watch(priv->io_out, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
		priv->err_id = g_io_add_watch(priv->io_err, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
		#else
		close(outfd);
		close(errfd);
		#endif
		return TRUE;
	}
}

// --
// Stop daemon
// --
static gboolean
lastfm_submit_disable(EinaLastfmPlugin *plugin)
{
	g_return_val_if_fail(EINA_IS_LASTFM_PLUGIN(plugin), FALSE);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	if (!priv->daemonpid)
		return TRUE;

	// Kill daemon
	if (priv->daemonpid)
	{
		GPid pid = priv->daemonpid;
		priv->daemonpid = 0;
		kill(pid, 15);
		#if DEBUG_DAEMON
		g_warning("Daemon stopped");
		#endif
	}

	// Remove watchers
	if (priv->out_id)
	{
		g_source_remove(priv->out_id);
		priv->out_id = 0;
	}
	if (priv->err_id)
	{
		g_source_remove(priv->err_id);
		priv->err_id = 0;
	}

	// Close channles
	if (priv->io_out)
	{
		g_io_channel_shutdown(priv->io_out, FALSE, NULL);
		priv->io_out = NULL;
	}
	if (priv->io_err)
	{
		g_io_channel_shutdown(priv->io_err, FALSE, NULL);
		priv->io_err = NULL;
	}
	return TRUE;
}

// --
// Submit song
// --
static void
lastfm_submit_submit(EinaLastfmPlugin *plugin)
{
	g_return_if_fail(EINA_IS_LASTFM_PLUGIN(plugin));
	EinaLastfmPluginPrivate *priv = plugin->priv;

	LomoStream *stream = lomo_player_get_current_stream(priv->lomo);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	// Build YAML format
	gchar *tmp = gel_str_parser("---\n{album: %b\n}{title: %t\n}{artist: %a\n}", (GelStrParserFunc) str_parser_cb, stream);
	GString *str = g_string_new(tmp);
	g_free(tmp);

	g_string_append_printf(str, "length: %"G_GINT64_FORMAT":%02"G_GINT64_FORMAT"\n",
		LOMO_NANOSECS_TO_SECS(lomo_player_get_length(priv->lomo)) / 60,
		LOMO_NANOSECS_TO_SECS(lomo_player_get_length(priv->lomo)) % 60);

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

// --
// Create config for lastfmsubmitd
// --
static gboolean
lastfm_submit_write_config(EinaLastfmPlugin *plugin)
{
	g_return_val_if_fail(EINA_IS_LASTFM_PLUGIN(plugin), FALSE);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	const gchar *username = g_settings_get_string(priv->settings, LASTFM_USERNAME_KEY);
	const gchar *password = g_settings_get_string(priv->settings, LASTFM_PASSWORD_KEY);
	if (!username || !password)
	{
		lastfm_submit_disable(plugin);
		return FALSE;
	}

	// Build contents
	const gchar *cache = g_get_user_cache_dir();
	gchar *contents = g_strdup_printf(
		"[paths]\n"
		"spool=%s/%s/lastfmsubmitd/spool\n"
		"pidfile=%s/%s/lastfmsubmitd/pid\n"
		"log=%s/%s/lastfmsubmitd/log\n"
		"\n"
		"[account]\n"
		"user=%s\n"
		"password=%s\n",
		cache, PACKAGE,
		cache, PACKAGE,
		cache, PACKAGE,
		username, password);

	// Write file
	gchar *dirname = g_build_filename(g_get_home_dir(), ".lastfmsubmitd", NULL);
	GError *err = NULL;
	if (!eina_fs_mkdir(dirname, 0700, &err))
	{
		g_warning("Cannot create conffile: %s", err->message);
		g_error_free(err);
		g_free(dirname);
		lastfm_submit_disable(plugin);
		return FALSE;
	}

	gchar *pathname = g_build_filename(dirname, "conf", NULL);
	g_free(dirname);

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

	// (Re)Start daemon if enabled
	if (g_settings_get_boolean(priv->settings, LASTFM_SUBMIT_ENABLED_KEY))
	{
		lastfm_submit_disable(plugin);
		lastfm_submit_enable (plugin);
	}

	return TRUE;
}

// --
// Scheduling updates
// --
static void
lastfm_submit_sched_write_config(EinaLastfmPlugin *plugin)
{
	g_return_if_fail(EINA_IS_LASTFM_PLUGIN(plugin));
	EinaLastfmPluginPrivate *priv = plugin->priv;

	if (priv->config_update_id > 0)
		g_source_remove(priv->config_update_id);
	priv->config_update_id = g_timeout_add_seconds(5, (GSourceFunc) lastfm_submit_write_config_wrapper, plugin);
}

static gboolean
lastfm_submit_write_config_wrapper(EinaLastfmPlugin *plugin)
{
	g_return_val_if_fail(EINA_IS_LASTFM_PLUGIN(plugin), FALSE);
	EinaLastfmPluginPrivate *priv = plugin->priv;

	if (priv->config_update_id > 0)
		g_source_remove(priv->config_update_id);
	priv->config_update_id = 0;
	lastfm_submit_write_config(plugin);
	return FALSE;
}

// --
// Callbacks
// --
static gchar *
str_parser_cb(gchar key, LomoStream *stream)
{
	const gchar *tag = LOMO_TAG_INVALID;
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
	return lomo_stream_strdup_tag_value(stream, tag);
}

#if DEBUG_DAEMON
static gboolean
io_watch_cb(GIOChannel *io, GIOCondition cond, EinaLastfmPlugin *plugin)
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
				lastfm_submit_disable(self);
				return FALSE;
			}
			return TRUE;
				
		default:
			lastfm_submit_disable(self);
			return FALSE;
		}
	}
	else if ((cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)))
	{
		lastfm_submit_disable(self);
		return FALSE;
	}
	else
	{
		lastfm_submit_disable(self);
		return FALSE;
	}
}
#endif

static void
settings_changed_cb(GSettings *settings, const gchar *key, EinaLastfmPlugin *plugin)
{
	if (g_str_equal(key, LASTFM_SUBMIT_ENABLED_KEY))
	{
		if (g_settings_get_boolean(settings, key))
			lastfm_submit_enable(plugin);
		else
			lastfm_submit_disable(plugin);
	}

	else if (g_str_equal(key, LASTFM_USERNAME_KEY) ||
	         g_str_equal(key, LASTFM_PASSWORD_KEY))
	{
		lastfm_submit_sched_write_config(plugin);
	}

	else
		g_warning(N_("Unknow key '%s'"), key);
}

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastfmPlugin *plugin)
{
	if (ev.type != LOMO_PLAYER_HOOK_CHANGE)
		return FALSE;

	LomoStream *stream = lomo_player_get_current_stream(lomo);
	if (!stream)
		return FALSE;

	gint total  = LOMO_NANOSECS_TO_SECS(lomo_player_get_length(lomo));
	gint played = LOMO_NANOSECS_TO_SECS(lomo_player_stats_get_stream_time_played(lomo));
	#if DEBUG_DAEMON
	g_warning("Got change. Played: %d:%02d (%d:%02d)", played / 60 , played % 60, total / 60 , total % 60);
	#endif

	if (total < 30)
		return FALSE;
	if ((played < 240) && (played < (total/2)))
		return FALSE;

	// Submit!
	lastfm_submit_submit(plugin);
	#if DEBUG_DAEMON
	g_warning("Submit to LastFM!");
	#endif

	return FALSE;
}
