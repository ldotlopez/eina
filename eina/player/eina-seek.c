/*
 * eina/player/eina-seek.c
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

#include "eina-seek.h"
#include <lomo/lomo-util.h>
#include <gel/gel.h>

G_DEFINE_TYPE (EinaSeek, eina_seek, GEL_UI_TYPE_SCALE)

enum {
	EINA_SEEK_TIME_CURRENT,
	EINA_SEEK_TIME_REMAINING,
	EINA_SEEK_TIME_TOTAL,
	EINA_SEEK_N_TIMES
};

enum {
	PROP_LOMO_PLAYER = 1,
	PROP_TIME_CURRENT_LABEL,
	PROP_TIME_REMAINING_LABEL,
	PROP_TIME_TOTAL_LABEL,
};

struct _EinaSeekPrivate {
	LomoPlayer *lomo;

	gint64    fast_scan_position;
	guint     fast_scan_timeout_id;

	guint     updater_id;
	gboolean  total_is_desync;

	GtkLabel *time_labels[EINA_SEEK_N_TIMES];
	gchar    *time_fmts[EINA_SEEK_N_TIMES];
	gchar    *time_def_val[EINA_SEEK_N_TIMES];
};

// --
// Internal funcions
// --
static void
seek_updater_start(EinaSeek *self);
static void
seek_updater_stop(EinaSeek *self);
static gboolean
updater_timeout_cb(EinaSeek *self);
static void
eina_seek_set_generic_label(EinaSeek *self, gint id, GtkLabel *label);
static void
seek_update_labels(EinaSeek *self, gint64 current_time, gint64 total_time, gboolean temp);
static gchar*
seek_fmt_time(EinaSeek *self, gint id, gint64 time, gboolean tempstr);


// --
// UI callbacks
// --
static void
value_changed_cb (GtkWidget *w, EinaSeek *self);
static gboolean
seek_fast_scan_cb(EinaSeek *self);
static gboolean
button_press_event_cb (GtkWidget *w, GdkEventButton *ev, EinaSeek *self);
static gboolean
button_release_event_cb (GtkWidget *w, GdkEventButton *ev, EinaSeek *self);

// --
// Lomo callbacks
// --
void
lomo_notify_current_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaSeek *self);
void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaSeek *self);

// --
// Get/Set properties
// --
static void
eina_seek_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	EinaSeek *self = EINA_SEEK(object);
	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		g_value_set_object(value, (gpointer) eina_seek_get_lomo_player(self));
		break;

	case PROP_TIME_CURRENT_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_current_label(self));
		break;

	case PROP_TIME_REMAINING_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_remaining_label(self));
		break;

	case PROP_TIME_TOTAL_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_total_label(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_seek_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	EinaSeek *self = EINA_SEEK(object);

	switch (property_id)
	{
	case PROP_LOMO_PLAYER:
		eina_seek_set_lomo_player(self, LOMO_PLAYER(g_value_get_object(value)));
		break;

	case PROP_TIME_CURRENT_LABEL:
		eina_seek_set_current_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	case PROP_TIME_REMAINING_LABEL:
		eina_seek_set_remaining_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	case PROP_TIME_TOTAL_LABEL:
		eina_seek_set_total_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

// --
// Dispose and finalize hooks
// --
static void
eina_seek_dispose (GObject *object)
{
	EinaSeek *self = EINA_SEEK(object);
	EinaSeekPrivate *priv = self->priv;

	if (priv->fast_scan_timeout_id)
	{
		g_source_remove(priv->fast_scan_timeout_id);
		priv->fast_scan_timeout_id = 0;
	}

	seek_updater_stop(self);

	for (guint i = 0; i < EINA_SEEK_N_TIMES; i++)
		eina_seek_set_generic_label(self, i, NULL);

	gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);

	G_OBJECT_CLASS (eina_seek_parent_class)->dispose (object);
}

// --
// Class init, init and new
// --
static void
eina_seek_class_init (EinaSeekClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaSeekPrivate));

	object_class->get_property = eina_seek_get_property;
	object_class->set_property = eina_seek_set_property;
	object_class->dispose = eina_seek_dispose;

	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "Lomo player", "LomoPlayer object to control/watch",
		LOMO_TYPE_PLAYER, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, PROP_TIME_CURRENT_LABEL,
		g_param_spec_object("current-label", "Current label", "GtkLabel widget to show current time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, PROP_TIME_REMAINING_LABEL,
		g_param_spec_object("remaining-label", "Remaining label", "GtkLabel widget to show remaining time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, PROP_TIME_TOTAL_LABEL,
		g_param_spec_object("total-label", "Total label", "GtkLabel widget to show total time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));
}

static void
eina_seek_init (EinaSeek *self)
{
	EinaSeekPrivate *priv = self->priv = (G_TYPE_INSTANCE_GET_PRIVATE ((self), EINA_TYPE_SEEK, EinaSeekPrivate));

	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_HORIZONTAL);

	priv->total_is_desync = TRUE;
	priv->lomo   = NULL;
	for (guint i = 0; i < EINA_SEEK_N_TIMES; i++)
	{
		priv->time_labels[i] = NULL;
		priv->time_fmts[i]   = NULL;
	}

	priv->fast_scan_position   = -1;
	priv->fast_scan_timeout_id = 0;
}

EinaSeek*
eina_seek_new (void)
{
	EinaSeek *self = g_object_new (EINA_TYPE_SEEK, NULL);

	// FIXME: Move to EinaSeek.init
	gtk_scale_set_draw_value(GTK_SCALE(self), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(self), FALSE);
	gtk_range_set_range(GTK_RANGE(self), 0, 1000);

	// Removed in gtk3, not sure if removing this creates any regression
	// gtk_range_set_update_policy(GTK_RANGE(self), GTK_UPDATE_CONTINUOUS);

	g_signal_connect(self, "value-changed",        G_CALLBACK(value_changed_cb), self);
	g_signal_connect(self, "button-press-event",   G_CALLBACK(button_press_event_cb), self);
	g_signal_connect(self, "button-release-event", G_CALLBACK(button_release_event_cb), self);

	return self;
}

// --
// Getters and setters
// --
void
eina_seek_set_lomo_player(EinaSeek *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_SEEK(self));

	if (lomo != NULL)
		g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaSeekPrivate *priv = self->priv;

	if (priv->lomo)
	{
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_notify_state_cb,   self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_notify_current_cb, self);
		g_object_unref(priv->lomo);
		priv->lomo = NULL;
	}

	if (lomo != NULL)
	{
		priv->lomo = g_object_ref(lomo);
		g_signal_connect(lomo, "notify::state",   (GCallback) lomo_notify_state_cb,   self);
		g_signal_connect(lomo, "notify::current", (GCallback) lomo_notify_current_cb, self);
	}
}

LomoPlayer *
eina_seek_get_lomo_player(EinaSeek *self)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	return self->priv->lomo;
}

static void
eina_seek_set_generic_label(EinaSeek *self, gint id, GtkLabel *label)
{
	g_return_if_fail(EINA_IS_SEEK(self));
	g_return_if_fail(id < EINA_SEEK_N_TIMES);
	if (label != NULL)
		g_return_if_fail(GTK_IS_LABEL(label));

	EinaSeekPrivate *priv = self->priv;

	priv->time_labels[id] = NULL;
	gel_free_and_invalidate(priv->time_labels[id],  NULL, g_object_unref);
	gel_free_and_invalidate(priv->time_fmts[id],    NULL, g_free);
	gel_free_and_invalidate(priv->time_def_val[id], NULL, g_free);

	if (label == NULL)
		return;

	g_object_ref(label);
	priv->time_labels[id] = label;
	priv->time_fmts[id]   = g_strdup(gtk_label_get_label(label));

	gchar *d = priv->time_def_val[id] = g_strdup_printf(priv->time_fmts[id], 0, 0);
	for (guint i = 0; d[i] != '\0'; i++)
		if (d[i] == '0')
			d[i] = '-';
	gtk_label_set_label(label, d);
}

void
eina_seek_set_current_label(EinaSeek *self, GtkLabel *label)
{
	eina_seek_set_generic_label(self, EINA_SEEK_TIME_CURRENT, label);
}

void
eina_seek_set_remaining_label(EinaSeek *self, GtkLabel *label)
{
	eina_seek_set_generic_label(self, EINA_SEEK_TIME_REMAINING, label);
}

void
eina_seek_set_total_label(EinaSeek *self, GtkLabel *label)
{
	eina_seek_set_generic_label(self, EINA_SEEK_TIME_TOTAL, label);
}

LomoPlayer*
eina_seek_get_lomo(EinaSeek *self)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	return self->priv->lomo;
}

GtkLabel*
eina_seek_get_current_label(EinaSeek *self)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	return self->priv->time_labels[EINA_SEEK_TIME_CURRENT];
}

GtkLabel*
eina_seek_get_remaining_label(EinaSeek *self)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	return self->priv->time_labels[EINA_SEEK_TIME_REMAINING];
}

GtkLabel*
eina_seek_get_total_label(EinaSeek *self)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	return self->priv->time_labels[EINA_SEEK_TIME_TOTAL];
}

// --
// Internal funcions
// --
void
seek_updater_start(EinaSeek *self)
{
	g_return_if_fail(EINA_IS_SEEK(self));
	EinaSeekPrivate *priv = self->priv;

	if (priv->updater_id > 0)
		g_source_remove(priv->updater_id);
	priv->updater_id = g_timeout_add(400, (GSourceFunc) updater_timeout_cb, (gpointer) self);
}

void
seek_updater_stop(EinaSeek *self)
{
	g_return_if_fail(EINA_IS_SEEK(self));
	EinaSeekPrivate *priv = self->priv;

	if (priv->updater_id > 0)
	{
		g_source_remove(priv->updater_id);
		priv->updater_id = 0;
	}
}

static gboolean
updater_timeout_cb(EinaSeek *self)
{
	EinaSeekPrivate *priv = self->priv;
	gint64 length = 0, position = 0;
	gdouble percent;

	length = lomo_player_get_length(priv->lomo);
	if (length == -1)
	{
		position = -1;
		percent  = 0;
	}
	else
	{
		position = lomo_player_get_position(priv->lomo);
		percent  = (gdouble)((position * 1000) / length);
	}

    g_signal_handlers_block_by_func(
		self,
		value_changed_cb,
		self);
	gtk_range_set_value(GTK_RANGE(self), percent);
	g_signal_handlers_unblock_by_func(
		self,
		value_changed_cb,
		self);

	seek_update_labels(self, position, length, FALSE);

	return TRUE;
}

static void
seek_update_labels(EinaSeek *self, gint64 current_time, gint64 total_time, gboolean temp)
{
	EinaSeekPrivate *priv = self->priv;
	gchar *current, *remaining, *total;

	// Sync total only if not synced
	if (priv->total_is_desync && (priv->time_labels[EINA_SEEK_TIME_TOTAL] != NULL))
	{
		if (total_time >= 0)
		{
			total = seek_fmt_time(self, EINA_SEEK_TIME_TOTAL, total_time, FALSE);
			gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_TOTAL], total);
			g_free(total);
			priv->total_is_desync = FALSE;
		}
		else
			gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_TOTAL], priv->time_def_val[EINA_SEEK_TIME_TOTAL]);
	}
	// If total is -1 even if there is no widget to show it, self must be
	// insensitive and progress reset
	/*
	if (total_time < 0)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(self), FALSE);
		gtk_range_set_value(GTK_RANGE(self), (gdouble) 0);
	}
	*/

	// Remaining
	if (priv->time_labels[EINA_SEEK_TIME_REMAINING])
	{
		if ((total_time >= 0) && (current_time >= 0) && (total_time >= current_time))
			remaining =  seek_fmt_time(self, EINA_SEEK_TIME_REMAINING, total_time - current_time, temp);
		else
			remaining = NULL;

		gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_REMAINING],
			remaining ? remaining : priv->time_def_val[EINA_SEEK_TIME_REMAINING]);
		gel_free_and_invalidate(remaining, NULL, g_free);
	}

	// Current
	if (priv->time_labels[EINA_SEEK_TIME_CURRENT])
	{
		if (current_time >= 0)
			current = seek_fmt_time(self, EINA_SEEK_TIME_CURRENT, current_time, temp);
		else
			current = NULL;
		gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_CURRENT],
			current ? current : priv->time_def_val[EINA_SEEK_TIME_CURRENT]);
		gel_free_and_invalidate(current, NULL, g_free);
   }
}

static gchar*
seek_fmt_time(EinaSeek *self, gint id, gint64 time, gboolean tempstr)
{
	g_return_val_if_fail(EINA_IS_SEEK(self), NULL);
	EinaSeekPrivate *priv = self->priv;

	if (time < 0)
		return NULL;

	gint secs = LOMO_NANOSECS_TO_SECS(time);

	gchar *ret = NULL;
	if (tempstr)
	{
		gchar *tmp = g_strdup_printf(priv->time_fmts[id], secs / 60, secs % 60);
		ret = g_strdup_printf("<i>%s</i>", tmp);
		g_free(tmp);
	}
	else
		ret = g_strdup_printf(priv->time_fmts[id], secs / 60, secs % 60);

	return ret;
}

// --
// UI Callbacks
// --
static void
value_changed_cb(GtkWidget *w, EinaSeek *self)
{
	EinaSeekPrivate *priv = self->priv;

	/*
	 * Stop the fastscan timeout function if any
	 */
	if (priv->fast_scan_timeout_id > 0)
		g_source_remove(priv->fast_scan_timeout_id);

	gint64 length = lomo_player_get_length(priv->lomo);
	gdouble value = gtk_range_get_value(GTK_RANGE(self));

	gint64 pseudo_position = length * (value / 1000);

	seek_update_labels(self, pseudo_position, length, TRUE);

	/* Create a timeout function */
	priv->fast_scan_position   = pseudo_position;
	priv->fast_scan_timeout_id = g_timeout_add(100, (GSourceFunc) seek_fast_scan_cb, self);
}

static gboolean
seek_fast_scan_cb(EinaSeek *self)
{
	EinaSeekPrivate *priv = self->priv;
	g_return_val_if_fail(LOMO_IS_PLAYER(priv->lomo), FALSE);

	LomoState state = lomo_player_get_state(priv->lomo);
	if ((state == LOMO_STATE_STOP) || (state == LOMO_STATE_INVALID))
		lomo_player_set_state(priv->lomo, LOMO_STATE_PLAY, NULL);

	lomo_player_set_position(priv->lomo, priv->fast_scan_position);

	priv->fast_scan_timeout_id =  0;
	priv->fast_scan_position   = -1;

	return FALSE;
}

static gboolean
button_press_event_cb(GtkWidget *w, GdkEventButton *ev, EinaSeek *self)
{
	seek_updater_stop(self);
	return FALSE;
}

static gboolean
button_release_event_cb(GtkWidget *w, GdkEventButton *ev, EinaSeek *self)
{
	seek_updater_start(self);
	return FALSE;
}

void
lomo_notify_current_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaSeek *self)
{
	static gint64 prev_current = -1;
	gint current = lomo_player_get_current(lomo);

	if (prev_current == current)
		return;

	// Force update
	self->priv->total_is_desync = TRUE;
	updater_timeout_cb(self);

	gtk_widget_set_sensitive(GTK_WIDGET(self), current != -1);
	prev_current = current;
}

void
lomo_notify_state_cb(LomoPlayer *lomo, GParamSpec *pspec, EinaSeek *self)
{
	static LomoState prev_state = LOMO_STATE_INVALID;
	LomoState state = lomo_player_get_state(lomo);

	g_return_if_fail(prev_state != state);

	if (state == LOMO_STATE_PLAY)
		seek_updater_start(self);
	else
		seek_updater_stop(self);
}


