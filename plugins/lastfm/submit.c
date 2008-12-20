#include <lomo/player.h>
#include <lomo/util.h>
#include "lastfm.h"

struct _LastFMSubmit {
	gint64 secs_required;
	gint64 secs_played;
	gint64 check_point;
	gchar *daemon_path;
	gchar *client_path;
} ;

static void
lastfm_submit_lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self);
static void
lastfm_submit_lomo_state_change_cb(LomoPlayer *lomo, EinaPlugin *self);
static void
lastfm_submit_lomo_eos_cb(LomoPlayer *lomo, EinaPlugin *self);

gboolean
lastfm_submit_init(EinaPlugin *plugin, GError **error)
{
	EINA_PLUGIN_DATA(plugin)->submit = g_new0(LastFMSubmit, 1);
	eina_plugin_attach_events(plugin,
		"change", lastfm_submit_lomo_change_cb,
		"play",   lastfm_submit_lomo_state_change_cb,
		"pause",  lastfm_submit_lomo_state_change_cb,
		"stop",   lastfm_submit_lomo_state_change_cb,
		"eos",    lastfm_submit_lomo_eos_cb,
		NULL);

	return TRUE;
}

gboolean
lastfm_submit_exit(EinaPlugin *plugin, GError **error)
{
	eina_plugin_deattach_events(plugin,
		"change", lastfm_submit_lomo_change_cb,
		"play",   lastfm_submit_lomo_state_change_cb,
		"pause",  lastfm_submit_lomo_state_change_cb,
		"stop",   lastfm_submit_lomo_state_change_cb,
		"eos",    lastfm_submit_lomo_eos_cb,
		NULL);
	g_free(EINA_PLUGIN_DATA(plugin)->submit);
	return TRUE;
}

static void
lastfm_submit_reset_count(EinaPlugin *self)
{
	LastFM *data = EINA_PLUGIN_DATA(self);

	gel_warn("Reset counters");
	data->submit->secs_required = G_MAXINT64;
	data->submit->secs_played   = 0;
	data->submit->check_point = 0;
}

static void
lastfm_submit_set_checkpoint(EinaPlugin *self, gint64 checkpoint, gboolean add_to_played)
{
	LastFM *data = EINA_PLUGIN_DATA(self);

	gel_warn("Set checkpoint at %lld, add: %s", checkpoint, add_to_played ? "Yes" : "No");
	if (add_to_played)
		data->submit->secs_played += (checkpoint - data->submit->check_point);
	data->submit->check_point = checkpoint;
	gel_warn("played: %lld", data->submit->secs_played);
}

static void
lastfm_submit_lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self)
{
	lastfm_submit_reset_count(self);
}

static void
lastfm_submit_lomo_state_change_cb(LomoPlayer *lomo, EinaPlugin *self)
{
	LastFM *data = EINA_PLUGIN_DATA(self);
	LomoState state = lomo_player_get_state(lomo);

	switch (state)
	{
	case LOMO_STATE_STOP:
		lastfm_submit_reset_count(self);
		break;
	case LOMO_STATE_PAUSE:
		lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), TRUE);
		break;
	case LOMO_STATE_PLAY:
		if (data->submit->secs_required == G_MAXINT64)
			data->submit->secs_required = lomo_nanosecs_to_secs(lomo_player_length_time(lomo)) / 2;
		lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), FALSE);
		break;
	case LOMO_STATE_INVALID:
		break;
	}
}

static void
lastfm_submit_lomo_eos_cb(LomoPlayer *lomo, EinaPlugin *self)
{
	LastFM *data = EINA_PLUGIN_DATA(self);
	
	LomoStream *stream;
	stream = (LomoStream *) lomo_player_get_current_stream(lomo);
	gchar *artist, *album, *title;
	gchar *cmdl, *tmp;

	lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), TRUE);

	if (data->submit->secs_played < data->submit->secs_required)
	{
		gel_warn("Not sending stream %p, insufficient seconds played (%"G_GINT64_FORMAT"/ %"G_GINT64_FORMAT")", stream,
			data->submit->secs_played, data->submit->secs_required);
		return;
	}

	artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	if (!artist || !title)
	{
		gel_error("Cannot submit stream %p, unavailable tags", stream);
		return;
	}

	cmdl = g_strdup_printf("/usr/lib/lastfmsubmitd/lastfmsubmit --artist \"%s\" --title \"%s\" --length %"G_GINT64_FORMAT,
		artist,
		title,
		data->submit->secs_required * 2);
	if (album)
	{
		tmp = g_strdup_printf("%s --album \"%s\"", cmdl, album);
		g_free(cmdl);
		cmdl = tmp;
	}
	gel_warn("EXEC %lld/%lld: '%s'", data->submit->secs_played, data->submit->secs_required, cmdl);
	g_spawn_command_line_async(cmdl, NULL);
}

