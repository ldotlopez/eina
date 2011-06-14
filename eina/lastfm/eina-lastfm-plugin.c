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
EINA_DEFINE_EXTENSION_HEADERS(EinaLastfmPlugin, eina_lastfm_plugin)

#define SCHED_TIMEOUT 5
#define DEBUG_DAEMON  0

static void     lastfm_plugin_build_preferences(EinaActivatable *activatable);
// static void     lastfm_plugin_unbuild_preferences(GelPlugin *plugin);

static gboolean lastfm_submit_enable      (EinaLastfmData *self);
static gboolean lastfm_submit_disable     (EinaLastfmData *self);
static void     lastfm_submit_submit      (EinaLastfmData *self);
static gboolean lastfm_submit_write_config(EinaLastfmData *self);

static void     lastfm_submit_sched_write_config  (EinaLastfmData *self);
static gboolean lastfm_submit_write_config_wrapper(EinaLastfmData *self);

static gchar *  str_parser_cb(gchar key, LomoStream *stream);
#if DEBUG_DAEMON
static gboolean io_watch_cb(GIOChannel *io, GIOCondition cond, EinaLastfmData *self);
#endif

static void settings_changed_cb(GSettings *settings, const gchar *key, EinaLastfmData *self);

static gboolean lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastfmData *self);

EINA_DEFINE_EXTENSION(EinaLastfmPlugin, eina_lastfm_plugin, EINA_TYPE_LASTFM_PLUGIN)

static gboolean
eina_lastfm_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaLastfmData *self = g_new0(EinaLastfmData, 1);
	eina_activatable_set_data(activatable, self);

	self->lomo = eina_application_get_lomo(app);
	lomo_player_hook_add(self->lomo, (LomoPlayerHook) lomo_hook_cb, self);

	self->settings   = eina_application_get_settings(app, LASTFM_PREFERENCES_DOMAIN);
	self->daemonpath = g_build_filename(eina_activatable_get_data_dir(activatable), "lastfmsubmitd", "lastfmsubmitd", NULL);

	lastfm_plugin_build_preferences(activatable);

	lastfm_submit_write_config(self);
	lastfm_submit_enable(self);

	return TRUE;
}

static gboolean
eina_lastfm_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	// lastfm_plugin_unbuild_preferences(plugin);
	EinaLastfmData *self = eina_activatable_steal_data(activatable);

	lastfm_submit_disable(self);
	lomo_player_hook_remove(self->lomo, (LomoPlayerHook) lomo_hook_cb);

	g_free(self->daemonpath);
	g_free(self);

	return TRUE;
}

static void
lastfm_plugin_build_preferences(EinaActivatable *activatable)
{
	EinaLastfmData *self = (EinaLastfmData *) eina_activatable_get_data(activatable);

	gchar *prefs_ui_path = g_build_filename(eina_activatable_get_data_dir(activatable), "preferences.ui", NULL);
	GtkWidget *prefs_ui = gel_ui_generic_new_from_file(prefs_ui_path);
	if (!prefs_ui)
	{
		g_warning(N_("Cannot create preferences UI"));
		g_free(prefs_ui_path);
		return;
	}
	g_free(prefs_ui_path);

	GdkPixbuf *pb = NULL;
	g_warning("Missing features here!!!");
	/*
	gchar *icon_filename = gel_plugin_get_resource(plugin, GEL_RESOURCE_TYPE_IMAGE, "lastfm.png");
	GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(icon_filename, 16, 16, TRUE, NULL);
	gel_free_and_invalidate(icon_filename, NULL, g_free);
	*/

	self->prefs_tab = eina_preferences_tab_new();
	g_object_set(self->prefs_tab,
		"widget", prefs_ui,
		"label-text", N_("Last FM"),
		"label-image",  NULL, // gtk_image_new_from_pixbuf(pb),
		NULL);
	gel_free_and_invalidate(pb, NULL, g_object_unref);

	eina_preferences_tab_bindv(self->prefs_tab,
		self->settings, LASTFM_SUBMIT_ENABLED_KEY,  LASTFM_SUBMIT_ENABLED_KEY,  "active",
		self->settings, LASTFM_USERNAME_KEY, LASTFM_USERNAME_KEY, "text",
		self->settings, LASTFM_PASSWORD_KEY, LASTFM_PASSWORD_KEY, "text",
		NULL);
	g_signal_connect(self->settings, "changed", (GCallback) settings_changed_cb, self);

	eina_application_add_preferences_tab(eina_activatable_get_application(activatable), self->prefs_tab);
}

/*
static void
lastfm_plugin_unbuild_preferences(GelPlugin *plugin)
{
	EinaLastfmData *self = eina_activatable_get_data(plugin);

	EinaPreferences *preferences = eina_plugin_get_preferences(plugin);
	
	g_object_weak_unref((GObject *) self->prefs_tab, (GWeakNotify) lastfm_weak_ref_cb, NULL);
	eina_preferences_remove_tab(preferences, self->prefs_tab);
	g_object_unref(self->prefs_tab);
	self->prefs_tab = NULL;
}
*/
// **********
// * Submit *
// **********

// --
// Start daemon
// --
static gboolean
lastfm_submit_enable(EinaLastfmData *self)
{
	if (self->daemonpid)
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
	gchar *cmdl[] = { "python", self->daemonpath, "--debug", "--no-daemon", NULL }; 
	if (!
		#if DEBUG_DAEMON
		g_spawn_async_with_pipes
		#else
		g_spawn_async_with_pipes
		#endif
		(g_get_current_dir(), cmdl, NULL,
		G_SPAWN_SEARCH_PATH,
		NULL, NULL,
		&(self->daemonpid),
		NULL, &outfd, &errfd,
		&err))
	{
		g_warning(N_("Cannot spawn daemon (%s): %s"), self->daemonpath, err->message);
		g_error_free(err);
		return FALSE;
	}
	else
	{
		#if DEBUG_DAEMON
		g_warning("Daemon started as %d", self->daemonpid);
		self->io_out = g_io_channel_unix_new(outfd);
		self->io_err = g_io_channel_unix_new(errfd);
		g_io_channel_set_close_on_unref(self->io_out, TRUE);
		g_io_channel_set_close_on_unref(self->io_err, TRUE);
		self->out_id = g_io_add_watch(self->io_out, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
		self->err_id = g_io_add_watch(self->io_err, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL, (GIOFunc) io_watch_cb, self);
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
lastfm_submit_disable(EinaLastfmData *self)
{
	if (!self->daemonpid)
		return TRUE;

	// Kill daemon
	if (self->daemonpid)
	{
		GPid pid = self->daemonpid;
		self->daemonpid = 0;
		kill(pid, 15);
		#if DEBUG_DAEMON
		g_warning("Daemon stopped");
		#endif
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

// --
// Submit song
// --
static void
lastfm_submit_submit(EinaLastfmData *self)
{
	LomoStream *stream = lomo_player_get_current_stream(self->lomo);
	g_return_if_fail(LOMO_IS_STREAM(stream));

	// Build YAML format
	gchar *tmp = gel_str_parser("---\n{album: %b\n}{title: %t\n}{artist: %a\n}", (GelStrParserFunc) str_parser_cb, stream);
	GString *str = g_string_new(tmp);
	g_free(tmp);

	g_string_append_printf(str, "length: %"G_GINT64_FORMAT":%02"G_GINT64_FORMAT"\n",
		LOMO_NANOSECS_TO_SECS(lomo_player_length_time(self->lomo)) / 60,
		LOMO_NANOSECS_TO_SECS(lomo_player_length_time(self->lomo)) % 60);

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
lastfm_submit_write_config(EinaLastfmData *self)
{
	const gchar *username = g_settings_get_string(self->settings, LASTFM_USERNAME_KEY);
	const gchar *password = g_settings_get_string(self->settings, LASTFM_PASSWORD_KEY);
	if (!username || !password)
	{
		lastfm_submit_disable(self);
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
		lastfm_submit_disable(self);
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
	if (g_settings_get_boolean(self->settings, LASTFM_SUBMIT_ENABLED_KEY))
	{
		lastfm_submit_disable(self);
		lastfm_submit_enable(self);
	}

	return TRUE;
}

// --
// Scheduling updates
// --
static void
lastfm_submit_sched_write_config(EinaLastfmData *self)
{
	if (self->config_update_id > 0)
		g_source_remove(self->config_update_id);
	self->config_update_id = g_timeout_add_seconds(5, (GSourceFunc) lastfm_submit_write_config_wrapper, self);
}

static gboolean
lastfm_submit_write_config_wrapper(EinaLastfmData *self)
{
	if (self->config_update_id > 0)
		g_source_remove(self->config_update_id);
	self->config_update_id = 0;
	lastfm_submit_write_config(self);
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
	const gchar *ret = lomo_stream_get_tag(stream, tag);
	return (gchar *) ret ? g_strdup(ret) : NULL;
}

#if DEBUG_DAEMON
static gboolean
io_watch_cb(GIOChannel *io, GIOCondition cond, EinaLastfmData *self)
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
settings_changed_cb(GSettings *settings, const gchar *key, EinaLastfmData *self)
{
	if (g_str_equal(key, LASTFM_SUBMIT_ENABLED_KEY))
	{
		if (g_settings_get_boolean(settings, key))
			lastfm_submit_enable(self);
		else
			lastfm_submit_disable(self);
	}

	else if (g_str_equal(key, LASTFM_USERNAME_KEY) ||
	         g_str_equal(key, LASTFM_PASSWORD_KEY))
	{
		lastfm_submit_sched_write_config(self);
	}

	else
		g_warning(N_("Unknow key '%s'"), key);
}

static gboolean
lomo_hook_cb(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaLastfmData *self)
{
	if (ev.type != LOMO_PLAYER_HOOK_CHANGE)
		return FALSE;

	LomoStream *stream = lomo_player_get_current_stream(lomo);
	if (!stream)
		return FALSE;

	gint total  = LOMO_NANOSECS_TO_SECS(lomo_player_length_time(lomo));
	gint played = LOMO_NANOSECS_TO_SECS(lomo_player_stats_get_stream_time_played(lomo));
	#if DEBUG_DAEMON
	g_warning("Got change. Played: %d:%02d (%d:%02d)", played / 60 , played % 60, total / 60 , total % 60);
	#endif

	if (total < 30)
		return FALSE;
	if ((played < 240) && (played < (total/2)))
		return FALSE;

	// Submit!
	lastfm_submit_submit(self);
	#if DEBUG_DAEMON
	g_warning("Submit to LastFM!");
	#endif

	return FALSE;
}
