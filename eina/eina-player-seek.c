#define GEL_DOMAIN "Eina::Player::Seek"
#include <gel/gel.h>
#include "eina-player-seek.h"

G_DEFINE_TYPE (EinaPlayerSeek, eina_player_seek, GTK_TYPE_HSCALE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_PLAYER_TYPE_SEEK, EinaPlayerSeekPrivate))

typedef struct _EinaPlayerSeekPrivate EinaPlayerSeekPrivate;

struct _EinaPlayerSeekPrivate
{
	GtkLabel   *time_w;
	LomoPlayer *lomo;
	guint       updater_id, real_id;
	gint64      pos;
};

void
on_player_seek_value_changed (GtkWidget *w, EinaPlayerSeek *self);

gboolean
on_player_seek_button_press_event (GtkWidget *w, GdkEventButton *ev, EinaPlayerSeek *self);

gboolean
on_player_seek_button_release_event (GtkWidget *w, GdkEventButton *ev, EinaPlayerSeek *self);

gboolean
on_player_seek_timeout(EinaPlayerSeek *self);

void
on_player_seek_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaPlayerSeek *self);

static void
eina_player_seek_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_seek_set_property (GObject *object, guint property_id,
  const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_seek_dispose (GObject *object)
{
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(object);
	
	eina_player_seek_updater_stop(EINA_PLAYER_SEEK(object));
	
	if (priv->lomo)
	{
		g_object_unref(priv->lomo);
		priv->lomo = NULL;
	}

	if (priv->time_w)
	{
		g_object_unref(priv->time_w);
		priv->time_w = NULL;
	}

	if (G_OBJECT_CLASS (eina_player_seek_parent_class)->dispose)
		G_OBJECT_CLASS (eina_player_seek_parent_class)->dispose (object);
}

static void
eina_player_seek_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (eina_player_seek_parent_class)->finalize)
		G_OBJECT_CLASS (eina_player_seek_parent_class)->finalize (object);
}

static void
eina_player_seek_class_init (EinaPlayerSeekClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPlayerSeekPrivate));

	object_class->get_property = eina_player_seek_get_property;
	object_class->set_property = eina_player_seek_set_property;
	object_class->dispose = eina_player_seek_dispose;
	object_class->finalize = eina_player_seek_finalize;
}

static void
eina_player_seek_init (EinaPlayerSeek *self)
{
}

EinaPlayerSeek*
eina_player_seek_new (void)
{
	EinaPlayerSeek *self = g_object_new (EINA_PLAYER_TYPE_SEEK, NULL);
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);

	g_object_set(G_OBJECT(self), "draw-value", FALSE, NULL);
	gtk_range_set_range(GTK_RANGE(self), 0, 1000);
	gtk_range_set_update_policy(GTK_RANGE(self), GTK_UPDATE_CONTINUOUS);
	g_signal_connect(self, "value-changed",
		G_CALLBACK(on_player_seek_value_changed), self);

	g_signal_connect(self, "button-press-event",
		G_CALLBACK(on_player_seek_button_press_event), self);

	g_signal_connect(self, "button-release-event",
		G_CALLBACK(on_player_seek_button_release_event), self);

	priv->lomo   = NULL;
	priv->time_w = NULL;
    priv->real_id = -1;
	return self;
}

void
eina_player_seek_set_label(EinaPlayerSeek *self, GtkLabel *label)
{
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(GTK_IS_LABEL(label));

	if (priv->time_w)
		g_object_unref(priv->time_w);
	priv->time_w = label;
	g_object_ref(priv->time_w);
}

void
eina_player_seek_set_lomo(EinaPlayerSeek *self, LomoPlayer *lomo)
{
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	if (priv->lomo) {
		/* Disconnect signals */
		/*
		g_signal_handlers_disconnect_by_func(priv->lomo, eina_player_seek_updater_start, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, eina_player_seek_updater_stop, self);
		g_signal_handlers_disconnect_by_func(priv->lomo, eina_player_seek_updater_stop, self);
		*/
		g_object_unref(priv->lomo);
	}
	priv->lomo = lomo;
	g_object_ref(lomo);

	/* Reconnect signals */
	g_signal_connect_swapped(lomo, "play",
		G_CALLBACK(eina_player_seek_updater_start), self);
	g_signal_connect_swapped(lomo, "pause",
		G_CALLBACK(eina_player_seek_updater_stop), self);
	g_signal_connect_swapped(lomo, "stop",
		G_CALLBACK(eina_player_seek_updater_stop), self);
	g_signal_connect(lomo, "change",
		G_CALLBACK(on_player_seek_lomo_change), self);
}

void eina_player_seek_reset(EinaPlayerSeek *self) {
	
}

void eina_player_seek_updater_start(EinaPlayerSeek *self) {
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	if (priv->updater_id >= 0)
		g_source_remove(priv->updater_id);
	priv->updater_id = g_timeout_add(400, (GSourceFunc) on_player_seek_timeout, (gpointer) self);
}

void eina_player_seek_updater_stop(EinaPlayerSeek *self) {
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	if (priv->updater_id >= 0)
		g_source_remove(priv->updater_id);
}

/*
 * Private functions
 */
gboolean eina_player_seek_real_seek(EinaPlayerSeek *self) {
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	g_return_val_if_fail(LOMO_IS_PLAYER(priv->lomo), FALSE);

	lomo_player_seek_time(priv->lomo, priv->pos);
	priv->pos = -1;

	return FALSE;
}

/*
 * Callbacks
 */
void on_player_seek_value_changed(GtkWidget *w, EinaPlayerSeek *self) {
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
	gint64  total;
	gint64  pseudo_pos;
	gdouble val;
	gchar  *markup;

	/* 
	 * Stop the timeout function if any 
	 * XXX: Figure out a better way to detect this
	 */
	if ((priv->pos != -1) && priv->real_id) {
		g_source_remove(priv->real_id);
	}

	total = lomo_player_length_time(priv->lomo);
	val = gtk_range_get_value(GTK_RANGE(self));
	pseudo_pos = total * (val/1000);

	if (priv->time_w)
	{
		markup = g_strdup_printf("<tt><i>%02d:%02d</i></tt>",
			(gint) (lomo_nanosecs_to_secs(pseudo_pos) / 60),
			(gint) (lomo_nanosecs_to_secs(pseudo_pos) % 60));
		gtk_label_set_markup(GTK_LABEL(priv->time_w), markup);
		g_free(markup);
	}

	/* Create a timeout function */
	priv->pos = pseudo_pos;
	priv->real_id = g_timeout_add(100, (GSourceFunc) eina_player_seek_real_seek, self);
}

gboolean on_player_seek_button_press_event(GtkWidget *w, GdkEventButton *ev, EinaPlayerSeek *self) {
	eina_player_seek_updater_stop(self);
	return FALSE;
}

gboolean on_player_seek_button_release_event(GtkWidget *w, GdkEventButton *ev, EinaPlayerSeek *self) {
	eina_player_seek_updater_start(self);
	return FALSE;
}

gboolean on_player_seek_timeout(EinaPlayerSeek *self) {
	EinaPlayerSeekPrivate *priv = GET_PRIVATE(self);
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
		on_player_seek_value_changed,
		self);
	gtk_range_set_value(GTK_RANGE(self), percent);
	g_signal_handlers_unblock_by_func(
		self,
		on_player_seek_value_changed,
		self);

	if (priv->time_w)
	{
		markup = g_strdup_printf("<tt>%02d:%02d</tt>", (gint) (lomo_nanosecs_to_secs(pos) / 60), (gint)(lomo_nanosecs_to_secs(pos) % 60));
		gtk_label_set_markup(GTK_LABEL(priv->time_w), markup);
		g_free(markup);
	}

	return TRUE;
}

void
on_player_seek_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaPlayerSeek *self)
{
	gtk_range_set_value(GTK_RANGE(self), (gdouble) 0);
}
