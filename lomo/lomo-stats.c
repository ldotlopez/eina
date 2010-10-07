#include "lomo-stats.h"
#include "lomo-util.h"

struct _LomoStats {
	LomoPlayer *player;

	gboolean    submited;
	gint64      played;
	gint64      check_point;
	gboolean    submit;
};

#define debug(...) do ; while(0)
// #define debug(...) g_warning(__VA_ARGS__)

void
lomo_stats_destroy_real(LomoStats *self, gboolean player_is_active);
static inline void
stats_reset_counters(LomoStats *self);
static inline void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add);

static void
lomo_weak_ref_cb(LomoStats *self, LomoPlayer *invalid_player);
static void
lomo_state_change_cb(LomoPlayer *lomo, LomoStats *self);
static void
lomo_eos_cb(LomoPlayer *lomo, LomoStats *self);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LomoStats *self);
static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LomoStats *self);

static struct {
	gchar *signal;
	gpointer handler;
} __signal_table[] = {
	{ "play",       lomo_state_change_cb },
	{ "pause",      lomo_state_change_cb },
	{ "stop",       lomo_state_change_cb },
	{ "pre-change", lomo_eos_cb      },
	{ "eos",        lomo_eos_cb      },
	{ "change",     lomo_change_cb   },
	{ "seek",       lomo_seek_cb     },
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

	return self;
}

void
lomo_stats_destroy(LomoStats *self)
{
	lomo_stats_destroy_real(self, TRUE);
}

void
lomo_stats_destroy_real(LomoStats *self, gboolean player_is_active)
{
	g_return_if_fail(self != NULL);
	if (player_is_active)
	{
		g_return_if_fail(LOMO_IS_PLAYER(self->player));

		for (guint i = 0; i < G_N_ELEMENTS(__signal_table); i++)
			g_signal_handlers_disconnect_by_func(self->player, __signal_table[i].handler, self);

		g_object_weak_unref((GObject *) self->player, (GWeakNotify) lomo_weak_ref_cb, NULL);
		g_object_unref(self->player);
	}
	g_free(self);
}

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

static inline void
stats_set_checkpoint(LomoStats *self, gint64 check_point, gboolean add)
{
	debug("Set checkpoint: %"G_GINT64_FORMAT" secs (%d)", lomo_nanosecs_to_secs(check_point), add);
	if (add)
		self->played += (check_point - self->check_point);
	self->check_point = check_point;
	debug("  Currently %"G_GINT64_FORMAT" secs played", lomo_nanosecs_to_secs(self->played));
}

static void
lomo_weak_ref_cb(LomoStats *self, LomoPlayer *invalid_player)
{
	lomo_stats_destroy_real(self, FALSE);
}

static void
lomo_state_change_cb(LomoPlayer *lomo, LomoStats *self)
{
	LomoState state = lomo_player_get_state(lomo);
	switch (state)
	{
	case LOMO_STATE_PLAY:
		// Set checkpoint without acumulate
		stats_set_checkpoint(self, lomo_player_tell_time(lomo), FALSE);
		break;

	case LOMO_STATE_STOP:
		debug("stop signal, position may be 0");
	
	case LOMO_STATE_PAUSE:
		// Add to counter secs from the last checkpoint
		stats_set_checkpoint(self, lomo_player_tell_time(lomo), TRUE);
		break;

	default:
		// Do nothing?
		debug("Unknow state. Be careful");
		break;
	}
}

static void
lomo_eos_cb(LomoPlayer *lomo, LomoStats *self)
{
	debug("Got EOS/PRE_CHANGE");
	stats_set_checkpoint(self, lomo_player_tell_time(lomo), TRUE);

	if ((self->played >= 30) && (self->played >= (lomo_player_length_time(lomo) / 2)) && !self->submited)
	{
		debug("Submit to lastfm");
		self->submited = TRUE;
	}
	else
		debug("Not enought to submit to lastfm");
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, LomoStats *self)
{
	stats_reset_counters(self);
}

static void
lomo_seek_cb(LomoPlayer *lomo, gint64 from, gint64 to, LomoStats *self)
{
	stats_set_checkpoint(self, from, TRUE);
	stats_set_checkpoint(self, to,   FALSE);
}


