/*
 * lomo/lomo-stats.c
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
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:lomo-stats
 * @short_description: #LomoPlayer stats
 * @see_also: #LomoPlayer
 *
 * LomoStat provides some statistics from #LomoPlayer
 **/

#include "lomo-stats.h"
#include <glib/gi18n.h>
#include "lomo.h"

#define DEBUG 0
#define DEBUG_PREFIX "LomoStats "

#if DEBUG
#	define debug(...) g_debug(DEBUG_PREFIX __VA_ARGS__)
#else
#	define debug(...) ;
#endif

struct _LomoStats {
	LomoPlayer *player;

	gboolean    submited;
	gint64      played;
	gint64      check_point;
	gboolean    submit;
};

static void
stats_destroy_real(LomoStats *self, gboolean player_is_active);
static void
stats_reset_counters(LomoStats *self);
static void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add);

static void
lomo_weak_ref_cb(LomoStats *self, LomoPlayer *invalid_player);
static void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, LomoStats *self);
static void
lomo_notify_current_cb(LomoPlayer *lomo, GParamSpec *pspec, LomoStats *self);
static void
lomo_eos_cb(LomoPlayer *lomo, LomoStats *self);
static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LomoStats *self);

static struct {
	gchar *signal;
	gpointer handler;
} __signal_table[] = {
	{ "notify::state",   lomo_notify_state_cb   },
	{ "notify::current", lomo_notify_current_cb },
	{ "pre-change",      lomo_eos_cb      },
	{ "eos",             lomo_eos_cb      },
	{ "seek",            lomo_seek_cb     },
};

LomoStats*
lomo_stats_watch(LomoPlayer *player)
{
	g_return_val_if_fail(LOMO_IS_PLAYER(player), NULL);

	LomoStats *self = g_new0(LomoStats, 1);

	self->player = player;
	g_object_weak_ref((GObject *) player, (GWeakNotify) lomo_weak_ref_cb, self);

	for (guint i = 0; i < G_N_ELEMENTS(__signal_table); i++)
		g_signal_connect(player, __signal_table[i].signal, (GCallback) __signal_table[i].handler, self);

	debug("Collecting statistics from LomoPlayer instance %p", self);
	return self;
}

void
lomo_stats_destroy(LomoStats *self)
{
	stats_destroy_real(self, TRUE);
}

void
stats_destroy_real(LomoStats *self, gboolean player_is_active)
{
	g_return_if_fail(self != NULL);
	if (player_is_active)
	{
		g_return_if_fail(LOMO_IS_PLAYER(self->player));
		debug("Stop collecting from %p", self->player);

		for (guint i = 0; i < G_N_ELEMENTS(__signal_table); i++)
			g_signal_handlers_disconnect_by_func(self->player, __signal_table[i].handler, self);

		g_object_weak_unref((GObject *) self->player, (GWeakNotify) lomo_weak_ref_cb, NULL);
		g_object_unref(self->player);
	}
	g_free(self);
}

/**
 * lomo_stats_get_time_played:
 * @self: A #LomoStats
 *
 * Gets how many time current stream has been played
 *
 * Returns: Time in microseconds
 **/
gint64
lomo_stats_get_time_played(LomoStats *self)
{
	return self->played;
}

static void
stats_reset_counters(LomoStats *self)
{
	debug("Reseting counters");
	self->submited = FALSE;
	self->played = 0;
	self->check_point = 0;
}

static void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add)
{
	debug("Set checkpoint: %"G_GINT64_FORMAT" secs (%s)",
		LOMO_NANOSECS_TO_SECS(check_point), add ? "adding" : "not adding");
	if (add)
		self->played += (check_point - self->check_point);
	self->check_point = check_point;
	debug("Currently %"G_GINT64_FORMAT" secs have been played", LOMO_NANOSECS_TO_SECS(self->played));
}

static void
lomo_weak_ref_cb(LomoStats *self, LomoPlayer *invalid_player)
{
	stats_destroy_real(self, FALSE);
}

static void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, LomoStats *self)
{
	if (!g_str_equal(pspec->name, "name"))
		return;

	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		// Set checkpoint without acumulate
		stats_set_checkpoint(self, lomo_player_get_position(lomo), FALSE);
		break;

	case LOMO_STATE_STOP:
		debug("Stop signal, position may be 0");

	case LOMO_STATE_PAUSE:
		// Add to counter secs from the last checkpoint
		stats_set_checkpoint(self, lomo_player_get_position(lomo), TRUE);
		break;

	default:
		g_warning(_("Unknow state. Expect undefined behaviour"));
		break;
	}
}

static void
lomo_notify_current_cb(LomoPlayer *lomo, GParamSpec *pspec, LomoStats *self)
{
	if (!g_str_equal(pspec->name, "current"))
		return;

	static gint from = -1;
	gint to = lomo_player_get_current(lomo);
	if ((from == -1) && (to == -1))
		return;

	stats_reset_counters(self);
}

static void
lomo_eos_cb(LomoPlayer *lomo, LomoStats *self)
{
	debug("Got EOS/PRE_CHANGE");
	stats_set_checkpoint(self, lomo_player_get_position(lomo), TRUE);

	if ((self->played >= 30) && (self->played >= (lomo_player_get_length(lomo) / 2)) && !self->submited)
	{
		debug("30 secs or more than 50%% of stream has been played, considering it played");
		self->submited = TRUE;
	}
	else
		debug("Stream has length less than 30 secs or less that 50%% of stream has been played, "
		      "considering it NOT played");
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LomoStats *self)
{
	stats_set_checkpoint(self, from, TRUE);
	stats_set_checkpoint(self, to,   FALSE);
}

