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
#	if DEBUG
#define debug(...) g_debug(DEBUG_PREFIX __VA_ARGS__)
#else
#	define debug(...) ;
#endif

G_DEFINE_TYPE (LomoStats, lomo_stats, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_PLAYER
};

struct _LomoStatsPrivate {
	/*< private >*/
	LomoPlayer *player;

	gboolean    submited;
	gint64      played;
	gint64      check_point;
	gboolean    submit;
};

static void
stats_set_player(LomoStats *self, LomoPlayer *player);

static void
stats_reset_counters(LomoStats *self);
static void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add);

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

static void
lomo_stats_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	LomoStats *self = LOMO_STATS(object);
	switch (property_id) {
	case PROP_PLAYER:
		g_value_set_object(value, lomo_stats_get_player(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_stats_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	LomoStats *self = LOMO_STATS(object);
	switch (property_id) {
	case PROP_PLAYER:
		stats_set_player(self, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
lomo_stats_dispose (GObject *object)
{
	LomoStats *self = LOMO_STATS(object);
	LomoStatsPrivate *priv = self->priv;

	g_warn_if_fail(LOMO_IS_PLAYER(priv->player));
	if (priv->player)
	{
		debug("Stop collecting from %p", self->player);

		for (guint i = 0; i < G_N_ELEMENTS(__signal_table); i++)
			g_signal_handlers_disconnect_by_func(priv->player, __signal_table[i].handler, self);

		g_object_unref(priv->player);
		priv->player = NULL;
	}

	G_OBJECT_CLASS (lomo_stats_parent_class)->dispose (object);
}

static void
lomo_stats_class_init (LomoStatsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (LomoStatsPrivate));

	object_class->get_property = lomo_stats_get_property;
	object_class->set_property = lomo_stats_set_property;
	object_class->dispose = lomo_stats_dispose;

	g_object_class_install_property(object_class, PROP_PLAYER,
		g_param_spec_object("player", "player", "player",
			LOMO_TYPE_PLAYER, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

static void
lomo_stats_init (LomoStats *self)
{
	self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), LOMO_TYPE_STATS, LomoStatsPrivate));
}

LomoStats*
lomo_stats_new (LomoPlayer *player)
{
	return g_object_new (LOMO_TYPE_STATS, "player", player, NULL);
}

static void
stats_set_player(LomoStats *self, LomoPlayer *player)
{
	g_return_if_fail(LOMO_IS_STATS(self));
	g_return_if_fail(LOMO_IS_PLAYER(player));

	LomoStatsPrivate *priv = self->priv;
	g_return_if_fail(priv->player == NULL);

	priv->player = player;

	for (guint i = 0; i < G_N_ELEMENTS(__signal_table); i++)
		g_signal_connect(player, __signal_table[i].signal, (GCallback) __signal_table[i].handler, self);

	debug("Collecting statistics from LomoPlayer instance %p", self);
}

/**
 * lomo_stats_get_player:
 * @self: A #LomoStats
 *
 * Gets the #LomoPlayer associated with @self
 *
 * Returns: (transfer none): The #LomoPlayer
 */
LomoPlayer*
lomo_stats_get_player(LomoStats *self)
{
	g_return_val_if_fail(LOMO_IS_STATS(self), NULL);
	return self->priv->player;
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
	g_return_val_if_fail(LOMO_IS_STATS(self), -1);
	return self->priv->played;
}

static void
stats_reset_counters(LomoStats *self)
{
	g_return_if_fail(LOMO_IS_STATS(self));
	LomoStatsPrivate *priv = self->priv;

	debug("Reseting counters");
	priv->submited = FALSE;
	priv->played = 0;
	priv->check_point = 0;
}

static void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add)
{
	g_return_if_fail(LOMO_IS_STATS(self));
	LomoStatsPrivate *priv = self->priv;

	debug("Set checkpoint: %"G_GINT64_FORMAT" secs (%s)",
		LOMO_NANOSECS_TO_SECS(check_point), add ? "adding" : "not adding");
	if (add)
		priv->played += (check_point - priv->check_point);
	priv->check_point = check_point;
	debug("Currently %"G_GINT64_FORMAT" secs have been played", LOMO_NANOSECS_TO_SECS(self->played));
}

static void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, LomoStats *self)
{
	g_return_if_fail(LOMO_IS_STATS(self));

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
	g_return_if_fail(LOMO_IS_STATS(self));

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
	g_return_if_fail(LOMO_IS_STATS(self));
	LomoStatsPrivate *priv = self->priv;

	debug("Got EOS/PRE_CHANGE");
	stats_set_checkpoint(self, lomo_player_get_position(lomo), TRUE);

	if ((priv->played >= 30) && (priv->played >= (lomo_player_get_length(lomo) / 2)) && !priv->submited)
	{
		debug("30 secs or more than 50%% of stream has been played, considering it played");
		priv->submited = TRUE;
	}
	else
		debug("Stream has length less than 30 secs or less that 50%% of stream has been played, "
		      "considering it NOT played");
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LomoStats *self)
{
	g_return_if_fail(LOMO_IS_STATS(self));

	stats_set_checkpoint(self, from, TRUE);
	stats_set_checkpoint(self, to,   FALSE);
}

