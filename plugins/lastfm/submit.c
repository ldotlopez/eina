/*
 * plugins/lastfm/submit.c
 *
 * Copyright (C) 2004-2009 Eina
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

#include "lastfm.h"
#include <lomo/lomo-util.h> // lomo_nanosecs_to_secs et al

struct _LastFMSubmit {
	gint64      length;
	gint64      played;
	gint64      check_point;
};


static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self);
static void
lomo_state_change_cb(LomoPlayer *lomo, LastFMSubmit *self);
static void
lomo_eos_cb(LomoPlayer *lomo, LastFMSubmit *self);
static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LastFMSubmit *self);

struct {
	gchar *signal;
	GCallback callback;
} signals[] = {
	{ "change", (GCallback) lomo_change_cb       },
	{ "change", (GCallback) lomo_change_cb       },
	{ "play",   (GCallback) lomo_state_change_cb },
	{ "pause",  (GCallback) lomo_state_change_cb },
	{ "stop",   (GCallback) lomo_state_change_cb },
	{ "eos",    (GCallback) lomo_eos_cb          },
	{ "seek",   (GCallback) lomo_seek_cb         }
};


static void
reset_counters(LastFMSubmit *self)
{
	gel_warn("Reset counters");
	self->length = G_MAXINT64;
	self->played = 0;
	self->check_point = 0;
}

static void
set_length(LastFMSubmit *self, gint64 length)
{
	gel_warn("Set length: %d secs", lomo_nanosecs_to_secs(length));
	self->length = length;
}

static void
set_checkpoint(LastFMSubmit *self, gint64 check_point, gboolean add)
{
	gel_warn("Set checkpoint: %d secs (%d)", lomo_nanosecs_to_secs(check_point), add);
	if (add)
		self->played += (check_point - self->check_point);
	self->check_point = check_point;
	gel_warn("Currently %d secs played", lomo_nanosecs_to_secs(self->played));
}

static gboolean
can_submit(LastFMSubmit *self)
{
	return ((lomo_nanosecs_to_secs(self->length) >= 30) && ((self->length / 2) >= self->played));
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

	EINA_PLUGIN_DATA(plugin)->submit = self;

	return TRUE;
}

gboolean
lastfm_submit_fini(GelApp *app, EinaPlugin *plugin, GError **error)
{
	LomoPlayer *lomo = GEL_APP_GET_LOMO(app);
	g_return_val_if_fail(lomo != NULL, FALSE);

	LastFMSubmit *self = EINA_PLUGIN_DATA(plugin)->submit;
	gint i;
	for ( i = 0; i < G_N_ELEMENTS(signals); i++)
		g_signal_handlers_disconnect_by_func(lomo, signals[i].callback, self);

	g_free(EINA_PLUGIN_DATA(plugin)->submit);
	return TRUE;
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LastFMSubmit *self)
{
	reset_counters(self);
}

static void
lomo_state_change_cb(LomoPlayer *lomo, LastFMSubmit *self)
{
	LomoState state = lomo_player_get_state(lomo);

	switch (state)
	{
	case LOMO_STATE_STOP:
		// Reset counters
		reset_counters(self);
		break;

	case LOMO_STATE_PAUSE:
		// Set checkpoint adding
		set_checkpoint(self, lomo_player_tell_time(lomo), TRUE);
		break;

	case LOMO_STATE_PLAY:
		// State change to play but we still have no lenght
		if (self->length == G_MAXINT64)
			set_length(self, lomo_player_length_time(lomo));

		// Set checkpoint without adding
		set_checkpoint(self, lomo_player_tell_time(lomo), FALSE);
		break;

	case LOMO_STATE_INVALID:
		break;
	}
}

static void
lomo_eos_cb(LomoPlayer *lomo, LastFMSubmit *self)
{
	LomoStream *stream;
	stream = (LomoStream *) lomo_player_get_current_stream(lomo);

	// Got EOS, set checkpoint at end
	set_checkpoint(self, self->length, TRUE);

	gel_warn("Submitting, played %"G_GINT64_FORMAT" seconds of %"G_GINT64_FORMAT,
		self->played, self->length);
	if (can_submit(self))
		gel_warn("Submit!");

	/*
	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	gchar *title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
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
	*/
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LastFMSubmit *self)
{
	gel_warn((lomo_player_get_state(lomo) == LOMO_STATE_PLAY) ? "Playing" : "Paused");
	set_checkpoint(self, from, TRUE);
	set_checkpoint(self, to,   FALSE);
}
