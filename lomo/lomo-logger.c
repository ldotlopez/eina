#include <lomo/lomo-player.h>

static gchar*
format_stream(LomoStream *stream)
{
	gchar *unscape = g_uri_unescape_string(lomo_stream_get_tag(stream, LOMO_TAG_URI), NULL);
	gchar *ret = g_path_get_basename(unscape);
	g_free(unscape);
	return ret;
}

static void
notify_cb(LomoPlayer *lomo, GParamSpec *pspec, gpointer data)
{
	GValue value = {0};

	g_value_init(&value, pspec->value_type);
	g_object_get_property((GObject *) lomo, pspec->name, &value);
	gchar *value_str = g_strdup_value_contents(&value);

	g_debug("notify::%s [%s]", pspec->name, value_str ? value_str : "(unknow)");
	g_free(value_str);
}

void seek_cb
(LomoPlayer *self, gint64 old, gint64 new)
{
	g_debug("seek event [%"G_GINT64_FORMAT"] [%"G_GINT64_FORMAT"]", old, new);
}

void eos_cb
(LomoPlayer *self)
{
	g_debug("eos event");
}

void insert_cb
(LomoPlayer *self, LomoStream *stream, gint pos)
{
	gchar *t = format_stream(stream);
	g_debug("insert event [%s] [%d]", t, pos);
	g_free(t);
}

void remove_cb
(LomoPlayer *self, LomoStream *stream, gint pos)
{
	gchar *t = format_stream(stream);
	g_debug("remove event [%s] [%d]", t, pos);
	g_free(t);
}

void queue_cb
(LomoPlayer *self, LomoStream *stream, gint pos)
{
	gchar *t = format_stream(stream);
	g_debug("queue event [%s] [%d]", t, pos);
	g_free(t);
}

void dequeue_cb
(LomoPlayer *self, LomoStream *stream, gint pos)
{
	gchar *t = format_stream(stream);
	g_debug("dequeue event [%s] [%d]", t, pos);
	g_free(t);
}

void clear_cb
(LomoPlayer *self)
{
	g_debug("clear event");
}

void queue_clear_cb
(LomoPlayer *self)
{
	g_debug("queue-clear event");
}

void error_cb
(LomoPlayer *self, LomoStream *stream, GError *error)
{
	gchar *t = format_stream(stream);
	g_debug("error event [%s]", t);
	g_free(t);
}

void tag_cb
(LomoPlayer *self, LomoStream *stream, const gchar *tag)
{
	gchar *t = format_stream(stream);
	g_debug("tag event [%s] [%s]", t, tag);
	g_free(t);
}

void all_tags_cb
(LomoPlayer *self, LomoStream *stream)
{
	gchar *t = format_stream(stream);
	g_debug("all-tags event [%s]", t);
	g_free(t);
}

void pre_change_cb
(LomoPlayer *self)
{
	g_debug("pre-change event");
}

void change_cb
(LomoPlayer *self, gint from, gint to)
{
	g_debug("change event [%d] [%d]", from, to);
}

#ifdef LOMO_PLAYER_E_API
void repeat_cb
(LomoPlayer *self, gboolean val)
{
	g_debug("repeat event [%s]", val ? "TRUE" : "FALSE");
}
void random_cb
(LomoPlayer *self, gboolean val)
{
	g_debug("repeat event [%s]", val ? "TRUE" : "FALSE");
}

void state_changed_cb
(LomoPlayer *self)
{
	g_debug("state-change event");
}

void play_cb
(LomoPlayer *self)
{
	g_debug("play event");
}

void pause_cb
(LomoPlayer *self)
{
	g_debug("pause event");
}

void stop_cb
(LomoPlayer *self)
{
	g_debug("stop event");
}
void volume_cb
(LomoPlayer *self, gint volume)
{
	g_debug("volume event [%d]", volume);
}

void mute_cb
(LomoPlayer *self, gboolean mute)
{
	g_debug("mute event [%s]", mute ? "TRUE" : "FALSE");
}
#endif

void
lomo_player_start_logger(LomoPlayer *lomo)
{
	struct {
		gchar *name;
		GCallback callback;
	} table[] = {
		{ "notify", (GCallback) notify_cb},
		{ "seek", (GCallback) seek_cb },
		{ "eos", (GCallback) eos_cb},
		{ "insert", (GCallback) insert_cb},
		{ "remove", (GCallback) remove_cb},
		{ "queue", (GCallback) queue_cb},
		{ "dequeue", (GCallback) dequeue_cb},
		{ "clear", (GCallback) clear_cb},
		{ "queue-clear", (GCallback) queue_clear_cb},
		{ "error", (GCallback) error_cb},
		{ "tag", (GCallback) tag_cb},
		{ "all-tags", (GCallback) all_tags_cb},
		{ "pre-change", (GCallback) pre_change_cb },
		{ "change", (GCallback) change_cb}
		#ifdef LOMO_TAG_URI
		,
		{ "repeat", (GCallback) repeat_cb},
		{ "random",         (GCallback) random_cb},
		{ "state-changed", (GCallback) state_changed_cb},
		{ "play", (GCallback) play_cb},
		{ "pause", (GCallback) pause_cb },
		{ "stop", (GCallback) stop_cb},
		{ "volume", (GCallback) volume_cb},
		{ "mute", (GCallback) mute_cb}
		#endif
	};
	for (guint i = 0; i < G_N_ELEMENTS(table); i++)
		g_signal_connect(lomo, table[i].name, (GCallback) table[i].callback, NULL);
}
