/*
 * plugins/lastfm/submit.c
 *
 * Copyright (C) 2004-2010 Eina
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
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <glib/gstdio.h>
#include <lomo/lomo-util.h> // lomo_nanosecs_to_secs et al
#include "submit.h"
#define debug(...) ;

#define ALWAYS_GEN_YAML 0
#define SETTINGS_PATH "/plugins/lastfm"

/*
*** Submit procedure using lastfmsubmitd ***

** Create a conffile in ~/.lastfmsubmitd/conf (expand ~ to user's dir) **
[paths]
spool=~/.cache/eina/lastfmsubmitd/spool
pidfile=~/.cache/eina/lastfmsubmitd/pid
log=~/.cache/eina/lastfmsubmitd/log
 
[account]
user=*****
password=*****

** Start daemon in foreground to better handling **
$plugindir/lastfmsubmitd/lastfmsubmitd --no-daemon --debug

** Generate YAML files on spooldir when convenient
generate_yaml()

*/

struct _LastFMSubmit {
	gchar      *daemonpath;
	GPid        daemonpid;

	GIOChannel *io_out, *io_err;
	guint       out_id, err_id;

	gboolean    submited;
	gint64      played;
	gint64      check_point;

	gboolean    submit;
};

static gboolean 
daemon_start(LastFMSubmit *self, GError **error);
static gboolean
daemon_stop(LastFMSubmit *self, GError **error);

static inline void
set_checkpoint(LastFMSubmit *self, gint64 check_point, gboolean add);
static inline void
reset_counters(LastFMSubmit *self);

static gboolean
io_watch_cb(GIOChannel *io, GIOCondition cond, LastFMSubmit *self);

static void
lomo_state_change_cb(LomoPlayer *lomo, LastFMSubmit *self);
static void
lomo_eos_cb(LomoPlayer *lomo, LastFMSubmit *self);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self);
static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LastFMSubmit *self);

struct {
	gchar *signal;
	GCallback callback;
} signals[] = {
	{ "play",       (GCallback) lomo_state_change_cb  },
	{ "pause",      (GCallback) lomo_state_change_cb  },
	{ "stop",       (GCallback) lomo_state_change_cb  },
	{ "pre-change", (GCallback) lomo_eos_cb           },
	{ "eos",        (GCallback) lomo_eos_cb           },
	{ "change",     (GCallback) lomo_change_cb        },
	{ "seek",       (GCallback) lomo_seek_cb          }
};

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

static void
generate_yaml(LastFMSubmit *self, LomoPlayer *lomo, LomoStream *stream)
{
	if (self->submited || !self->submit)
		return;

	self->submited = TRUE;

	// Build YAML format
	gchar *tmp = gel_str_parser("---\n{album: %b\n}{title: %t\n}{artist: %a\n}", (GelStrParserFunc) str_parser_cb, stream);
	GString *str = g_string_new(tmp);
	g_free(tmp);

	g_string_append_printf(str, "length: %"G_GINT64_FORMAT":%02"G_GINT64_FORMAT"\n",
		lomo_nanosecs_to_secs(lomo_player_length_time(lomo)) / 60,
		lomo_nanosecs_to_secs(lomo_player_length_time(lomo)) % 60);

	struct tm *gmt;
	time_t     curtime = time (NULL);
	gmt = gmtime(&curtime);

	gchar strf[20];
	strftime(strf, 20, "%Y-%m-%d %H:%M:%S", gmt);
	g_string_append_printf(str, "time: !timestamp %s\n", strf);

	// Write to file
	gchar *spool = g_build_filename(g_get_user_cache_dir(), "eina", "lastfmsubmitd", "spool", NULL);
	if (!eina_fs_mkdir(spool, 0700))
	{
		gel_error("Cannot create spooldir");
		g_free(spool);
		g_string_free(str, TRUE);
		return;
	}

	gchar *spoolfile = g_build_filename(spool, "einalastfmXXXXXX", NULL);
	gint fd = g_mkstemp(spoolfile);
	if (fd < 0)
	{
		gel_error("Cannot open tempfile %s", spoolfile);
		g_free(spool);
		g_free(spoolfile);
		return;
	}
	if (write(fd, str->str, str->len) != str->len)
		gel_warn("File not completly writen");
	close(fd);

	g_string_free(str, TRUE);
	g_free(spool);
	g_free(spoolfile);
}

gboolean
lastfm_submit_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	g_return_val_if_fail(lomo != NULL, FALSE);

	LastFMSubmit *self = g_new0(LastFMSubmit, 1);
	reset_counters(self);
	gint i;
	for ( i = 0; i < G_N_ELEMENTS(signals); i++)
		g_signal_connect(lomo, signals[i].signal, signals[i].callback, self);

	self->daemonpath = gel_plugin_get_resource(plugin, GEL_RESOURCE_OTHER, "lastfmsubmitd/lastfmsubmitd");

	EINA_PLUGIN_DATA(plugin)->submit = self;

	return TRUE;
}

gboolean
lastfm_submit_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	g_return_val_if_fail(lomo != NULL, FALSE);

	LastFMSubmit *self = EINA_PLUGIN_DATA(plugin)->submit;

	GError *err = NULL;
	if (!daemon_stop(self, &err))
	{
		gel_error("Cannot stop daemon: %s", err->message);
		g_error_free(err);
	}

	gint i;
	for ( i = 0; i < G_N_ELEMENTS(signals); i++)
		g_signal_handlers_disconnect_by_func(lomo, signals[i].callback, self);

	g_free(EINA_PLUGIN_DATA(plugin)->submit->daemonpath);
	g_free(EINA_PLUGIN_DATA(plugin)->submit);

	return TRUE;
}

gboolean
lastfm_submit_set_account_info(LastFMSubmit *self, gchar *username, gchar *password)
{
	if ((username == NULL) || (password == NULL))
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
	const gchar *prgname = "eina";

	gchar *dirname = g_build_filename(g_get_home_dir(), ".lastfmsubmitd", NULL);
	if (!eina_fs_mkdir(dirname, 0700))
	{
		gel_error("Cannot create conffile");
		g_free(dirname);
		return FALSE;
	}
	
	gchar *pathname = g_build_filename(dirname, "conf", NULL);
	g_free(dirname);

	gchar *contents = g_strdup_printf(template,
		cache, prgname,
		cache, prgname,
		cache, prgname,
		username, password);
	GError *error = NULL;
	if (!g_file_set_contents(pathname, contents, -1, &error))
	{
		gel_error("Cannot create conffile %s: %s", pathname, error->message);
		g_error_free(error);
		g_free(pathname);
		g_free(contents);
		return FALSE;
	}
	g_free(pathname);
	g_free(contents);
	
	if (self->submit && (!daemon_stop(self, &error) || !daemon_start(self, &error)))
	{
		gel_error("Error restaring daemon: %s", error->message);
		g_error_free(error);
	}

	return TRUE;
}

gboolean
lastfm_submit_get_submit(LastFMSubmit *self)
{
	return self->submit;
}

void
lastfm_submit_set_submit(LastFMSubmit *self, gboolean submit)
{
	if (submit == self->submit)
		return;

	GError *error = NULL;
	gboolean ok;
	if (submit)
		ok = daemon_start(self, &error);
	else
		ok = daemon_stop(self, &error);

	if (ok)
		self->submit = submit;
	else
	{
		gel_error(N_("Cannot %s daemon: %s"), submit ? N_("enable") : N_("disable"), error->message);
		g_error_free(error);
	}
}

static gboolean
daemon_start(LastFMSubmit *self, GError **error)
{
	if (self->daemonpid != 0)
	{
		g_set_error(error, lastfm_quark(), EINA_LASTFM_ERROR_START_DAEMON,
			N_("There is a daemon running with PID: %d"), self->daemonpid);
		return FALSE;
	}

	// Folders
	gchar *spool = g_build_filename(g_get_user_cache_dir(), "eina", "lastfmsubmitd", "spool", NULL);
	if (!eina_fs_mkdir(spool, 0700))
	{
		g_set_error(error, lastfm_quark(), EINA_LASTFM_ERROR_START_DAEMON,
			N_("Cannot create spool folder (%s)"), spool);
		g_free(spool);
		return FALSE;
	}
	g_free(spool);

	gint outfd, errfd;
	GError *err = NULL;
	gchar *cmdl[] = { "python", self->daemonpath, "--debug", "--no-daemon", NULL }; 
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
		gel_warn("Daemon started as %d", self->daemonpid);
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
daemon_stop(LastFMSubmit *self, GError **error)
{
	if (!self->daemonpid)
		return TRUE;

	// Kill daemon
	if (self->daemonpid)
	{
		GPid pid = self->daemonpid;
		self->daemonpid = 0;
		kill(pid, 15);
		gel_warn("Daemon stopped");
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

static inline void
reset_counters(LastFMSubmit *self)
{
	debug("Reseting counters");
	self->submited = FALSE;
	self->played = 0;
	self->check_point = 0;
}

static inline void
set_checkpoint(LastFMSubmit *self, gint64 check_point, gboolean add)
{
	debug("Set checkpoint: %d secs (%d)", lomo_nanosecs_to_secs(check_point), add);
	if (add)
		self->played += (check_point - self->check_point);
	self->check_point = check_point;
	debug("  Currently %d secs played", lomo_nanosecs_to_secs(self->played));
}

static gboolean
io_watch_cb(GIOChannel *io, GIOCondition cond, LastFMSubmit *self)
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
					gel_error("%s", buff);
				g_free(buff);
				return TRUE;
			}
			if (error)
			{
				gel_error(error->message);
				g_error_free(error);
				daemon_stop(self, NULL);
				return FALSE;
			}
			return TRUE;
				
		default:
			daemon_stop(self, NULL);
			return FALSE;
		}
	}
	else if ((cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)))
	{
		daemon_stop(self, NULL);
		return FALSE;
	}
	else
	{
		daemon_stop(self, NULL);
		return FALSE;
	}
}

static void
lomo_state_change_cb(LomoPlayer *lomo, LastFMSubmit *self)
{
	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		// Set checkpoint without acumulate
		set_checkpoint(self, lomo_player_tell_time(lomo), FALSE);
		break;

	case LOMO_STATE_STOP:
		debug("stop signal, position may be 0");

	case LOMO_STATE_PAUSE:
		// Add to counter secs from the last checkpoint
		set_checkpoint(self, lomo_player_tell_time(lomo), TRUE);
		break;

	default:
		// Do nothing?
		debug("Unknow state. Be careful");
		break;
	}
}

static void
lomo_eos_cb(LomoPlayer *lomo, LastFMSubmit *self)
{
	debug("Got EOS/PRE_CHANGE");
	set_checkpoint(self, lomo_player_tell_time(lomo), TRUE);

#if ALWAYS_GEN_YAML
	// For debugging purposes
	generate_yaml(self, lomo, lomo_player_get_current_stream(lomo));

#else
	if ((self->played >= 30) && (self->played >= (lomo_player_length_time(lomo) / 2)) && !self->submited)
	{
		generate_yaml(self, lomo, lomo_player_get_current_stream(lomo));
		gel_warn("Submit to lastfm");
	}
	else
		debug("Not enought to submit to lastfm");
#endif
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self)
{
	reset_counters(self);
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LastFMSubmit *self)
{
	// Count from checkpoint to 'from'
	// Move checkpoint to 'to' without adding
	set_checkpoint(self, from, TRUE);
	set_checkpoint(self, to,   FALSE);
}

