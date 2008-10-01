#define GEL_DOMAIN "Eina::Player::Seek"
#include "eina-seek.h"
#include <lomo/util.h>
#include <gel/gel.h>

G_DEFINE_TYPE (EinaSeek, eina_seek, GTK_TYPE_HSCALE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_SEEK, EinaSeekPrivate))

typedef struct _EinaSeekPrivate EinaSeekPrivate;

enum {
	EINA_SEEK_TIME_CURRENT,
	EINA_SEEK_TIME_REMAINING,
	EINA_SEEK_TIME_TOTAL,
	EINA_SEEK_N_TIMES
};

enum {
	EINA_SEEK_PROPERTY_LOMO_PLAYER = 1,
	EINA_SEEK_PROPERTY_TIME_CURRENT_LABEL,
	EINA_SEEK_PROPERTY_TIME_REMAINING_LABEL,
	EINA_SEEK_PROPERTY_TIME_TOTAL_LABEL,
};

struct _EinaSeekPrivate {
	LomoPlayer *lomo;
	guint       updater_id, real_id;
	gint64      pos;
	gboolean    total_is_desync;

	GtkLabel *time_labels[EINA_SEEK_N_TIMES];
	gchar    *time_fmts[EINA_SEEK_N_TIMES];
};

static void
player_seek_value_changed_cb (GtkWidget *w, EinaSeek *self);

static gboolean
player_seek_button_press_event_cb (GtkWidget *w, GdkEventButton *ev, EinaSeek *self);

static gboolean
player_seek_button_release_event_cb (GtkWidget *w, GdkEventButton *ev, EinaSeek *self);

static gboolean
player_seek_timeout_cb(EinaSeek *self);

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaSeek *self);

static void
lomo_state_change_cb(LomoPlayer *lomo, EinaSeek *self);

static void
eina_seek_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaSeek *self = EINA_SEEK(object);
	switch (property_id) {

	case EINA_SEEK_PROPERTY_LOMO_PLAYER:
		g_value_set_object(value, (gpointer) eina_seek_get_lomo_player(self));
		break;

	case EINA_SEEK_PROPERTY_TIME_CURRENT_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_current_label(self));
		break;

	case EINA_SEEK_PROPERTY_TIME_REMAINING_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_remaining_label(self));
		break;

	case EINA_SEEK_PROPERTY_TIME_TOTAL_LABEL:
		g_value_set_object(value, (gpointer) eina_seek_get_total_label(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_seek_set_property (GObject *object, guint property_id,
  const GValue *value, GParamSpec *pspec)
{
	EinaSeek *self = EINA_SEEK(object);

	switch (property_id) {
	case EINA_SEEK_PROPERTY_LOMO_PLAYER:
		eina_seek_set_lomo_player(self, LOMO_PLAYER(g_value_get_object(value)));
		break;

	case EINA_SEEK_PROPERTY_TIME_CURRENT_LABEL:
		eina_seek_set_current_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	case EINA_SEEK_PROPERTY_TIME_REMAINING_LABEL:
		eina_seek_set_remaining_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	case EINA_SEEK_PROPERTY_TIME_TOTAL_LABEL:
		eina_seek_set_total_label(self, GTK_LABEL(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_seek_dispose (GObject *object)
{
	EinaSeekPrivate *priv = GET_PRIVATE(object);
	gint i;

	eina_seek_updater_stop(EINA_SEEK(object));
	
	if (priv->lomo)
	{
		g_object_unref(priv->lomo);
		priv->lomo = NULL;
	}

	for (i = 0; i < EINA_SEEK_N_TIMES; i++)
	{
		if (priv->time_labels[i])
			g_object_unref(priv->time_labels[i]);
		priv->time_labels[i] = NULL;
	}
	
	if (G_OBJECT_CLASS (eina_seek_parent_class)->dispose)
		G_OBJECT_CLASS (eina_seek_parent_class)->dispose (object);
}

static void
eina_seek_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (eina_seek_parent_class)->finalize)
		G_OBJECT_CLASS (eina_seek_parent_class)->finalize (object);
}

static void
eina_seek_class_init (EinaSeekClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaSeekPrivate));

	object_class->get_property = eina_seek_get_property;
	object_class->set_property = eina_seek_set_property;
	object_class->dispose = eina_seek_dispose;
	object_class->finalize = eina_seek_finalize;

	g_object_class_install_property(object_class, EINA_SEEK_PROPERTY_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "Lomo player", "LomoPlayer object to control/watch",
		LOMO_TYPE_PLAYER, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, EINA_SEEK_PROPERTY_TIME_CURRENT_LABEL,
		g_param_spec_object("current-label", "Current label", "GtkLabel widget to show current time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, EINA_SEEK_PROPERTY_TIME_REMAINING_LABEL,
		g_param_spec_object("remaining-label", "Remaining label", "GtkLabel widget to show remaining time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));

	g_object_class_install_property(object_class, EINA_SEEK_PROPERTY_TIME_TOTAL_LABEL,
		g_param_spec_object("total-label", "Total label", "GtkLabel widget to show total time",
		GTK_TYPE_LABEL, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));
}

static void
eina_seek_init (EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	gint i;

	priv->total_is_desync = TRUE;
	priv->lomo   = NULL;
	for (i = 0; i < EINA_SEEK_N_TIMES; i++)
		priv->time_labels[i] = NULL;
    priv->real_id = -1;
}

EinaSeek*
eina_seek_new (void)
{
	EinaSeek *self = g_object_new (EINA_TYPE_SEEK, NULL);

	g_object_set(G_OBJECT(self), "draw-value", FALSE, NULL);
	gtk_range_set_range(GTK_RANGE(self), 0, 1000);
	gtk_range_set_update_policy(GTK_RANGE(self), GTK_UPDATE_CONTINUOUS);

	g_signal_connect(self, "value-changed",
		G_CALLBACK(player_seek_value_changed_cb), self);
	g_signal_connect(self, "button-press-event",
		G_CALLBACK(player_seek_button_press_event_cb), self);
	g_signal_connect(self, "button-release-event",
		G_CALLBACK(player_seek_button_release_event_cb), self);

	return self;
}

void
eina_seek_updater_start(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	if (priv->updater_id > 0)
		g_source_remove(priv->updater_id);
	priv->updater_id = g_timeout_add(400, (GSourceFunc) player_seek_timeout_cb, (gpointer) self);
}

void
eina_seek_updater_stop(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	if (priv->updater_id > 0)
		g_source_remove(priv->updater_id);
}

void
eina_seek_set_lomo_player(EinaSeek *self, LomoPlayer *lomo)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	if ((lomo == NULL) || !LOMO_IS_PLAYER(lomo))
		return;

	if (priv->lomo)
	{
		g_signal_handlers_disconnect_by_func(priv->lomo, eina_seek_updater_start, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, eina_seek_updater_stop, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, lomo_state_change_cb, self);
		g_object_unref(priv->lomo);
	}

	priv->lomo = lomo;
	g_object_ref(lomo);

	/* Reconnect signals */
	g_signal_connect_swapped(lomo, "play",
		G_CALLBACK(eina_seek_updater_start), self);
	g_signal_connect_swapped(lomo, "pause",
		G_CALLBACK(eina_seek_updater_stop), self);
	g_signal_connect_swapped(lomo, "stop",
		G_CALLBACK(eina_seek_updater_stop), self);

	g_signal_connect(lomo, "play",
		G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(lomo, "pause",
		G_CALLBACK(lomo_state_change_cb), self);
	g_signal_connect(lomo, "stop",
		G_CALLBACK(lomo_state_change_cb), self);

	g_signal_connect(lomo, "change",
		G_CALLBACK(lomo_change_cb), self);
}

LomoPlayer *
eina_seek_get_lomo_player(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

static void
eina_seek_set_generic_label(EinaSeek *self, gint id, GtkLabel *label)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);

	if (label == NULL)
		return;
	g_return_if_fail(id < EINA_SEEK_N_TIMES);
	
	if (priv->time_labels[id])
		 g_object_unref(priv->time_labels[id]);
	g_object_ref(label);
	priv->time_labels[id] = label;
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
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

GtkLabel*
eina_seek_get_current_label(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	return priv->time_labels[EINA_SEEK_TIME_CURRENT];
}

GtkLabel*
eina_seek_get_remaining_label(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	return priv->time_labels[EINA_SEEK_TIME_REMAINING];
}

GtkLabel*
eina_seek_get_total_label(EinaSeek *self)
{
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	return priv->time_labels[EINA_SEEK_TIME_TOTAL];
}

/*
 * Private functions
 */
gboolean eina_seek_real_seek(EinaSeek *self) {
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(LOMO_IS_PLAYER(priv->lomo), FALSE);

	lomo_player_seek_time(priv->lomo, priv->pos);
	priv->pos = -1;

	return FALSE;
}

/*
 * Callbacks
 */
void player_seek_value_changed_cb(GtkWidget *w, EinaSeek *self) {
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	gint64  total;
	gint64  pseudo_pos;
	gdouble val;
	gchar  *markup;

	/* 
	 * Stop the timeout function if any 
	 * XXX: Figure out a better way to detect this
	 */
	if ((priv->pos != -1) && priv->real_id && (priv->real_id > 0)) {
		g_source_remove(priv->real_id);
	}

	total = lomo_player_length_time(priv->lomo);
	val = gtk_range_get_value(GTK_RANGE(self));
	pseudo_pos = total * (val/1000);

	// XXX: Handle remaining
	if (priv->time_labels[EINA_SEEK_TIME_CURRENT])
	{
		markup = g_strdup_printf("<small><tt><i>%02d:%02d</i></tt></small>",
			(gint) (lomo_nanosecs_to_secs(pseudo_pos) / 60),
			(gint) (lomo_nanosecs_to_secs(pseudo_pos) % 60));
		gtk_label_set_markup(GTK_LABEL(priv->time_labels[EINA_SEEK_TIME_CURRENT]), markup);
		g_free(markup);
	}

	/* Create a timeout function */
	priv->pos = pseudo_pos;
	priv->real_id = g_timeout_add(100, (GSourceFunc) eina_seek_real_seek, self);
}

gboolean player_seek_button_press_event_cb(GtkWidget *w, GdkEventButton *ev, EinaSeek *self) {
	eina_seek_updater_stop(self);
	return FALSE;
}

gboolean player_seek_button_release_event_cb(GtkWidget *w, GdkEventButton *ev, EinaSeek *self) {
	eina_seek_updater_start(self);
	return FALSE;
}

gboolean player_seek_timeout_cb(EinaSeek *self) {
	EinaSeekPrivate *priv = GET_PRIVATE(self);
	gint64 len = 0, pos = 0;
	gdouble percent;
	gchar *markup;
	
	len = lomo_player_length_time(priv->lomo);
	if ((lomo_player_get_state(priv->lomo) == LOMO_STATE_STOP) || len == 0 ) {
		percent = 0;
	} else {
		pos = lomo_player_tell_time(priv->lomo);
		percent = (gdouble)((pos * 1000) / len);
	}

    g_signal_handlers_block_by_func(
		self,
		player_seek_value_changed_cb,
		self);
	gtk_range_set_value(GTK_RANGE(self), percent);
	g_signal_handlers_unblock_by_func(
		self,
		player_seek_value_changed_cb,
		self);

	if (priv->time_labels[EINA_SEEK_TIME_CURRENT])
	{
		markup = g_strdup_printf("<small><tt>%02d:%02d</tt></small>", (gint) (lomo_nanosecs_to_secs(pos) / 60), (gint)(lomo_nanosecs_to_secs(pos) % 60));
		gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_CURRENT], markup);
		g_free(markup);
	}

	return TRUE;
}

void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaSeek *self)
{
	struct _EinaSeekPrivate *priv = GET_PRIVATE(self);
	priv->total_is_desync = TRUE;
	gtk_range_set_value(GTK_RANGE(self), (gdouble) 0);
}

void
lomo_state_change_cb(LomoPlayer *lomo, EinaSeek *self)
{
	struct _EinaSeekPrivate *priv = GET_PRIVATE(self);
	gint64 total = -1;
	gchar *markup;

	if (priv->time_labels[EINA_SEEK_TIME_TOTAL] == NULL)
		return;
	if (priv->total_is_desync == FALSE)
		return;
	total = lomo_player_length_time(lomo);
	if (total == -1)
		return;

	if (priv->time_labels[EINA_SEEK_TIME_TOTAL])
	{
		markup = g_strdup_printf("<small><tt>%02d:%02d</tt></small>", (gint) (lomo_nanosecs_to_secs(total) / 60), (gint)(lomo_nanosecs_to_secs(total) % 60));
		gtk_label_set_markup(priv->time_labels[EINA_SEEK_TIME_TOTAL], markup);
		g_free(markup);
	}
	priv->total_is_desync = FALSE;
}
