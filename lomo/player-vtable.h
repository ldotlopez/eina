#ifndef _LOMO_PLAYER_VTABLE_H
#define _LOMO_PLAYER_VTABLE_H

struct LomoPlayerFuncsVTable {
	/*
	 * Player functions
	 */
	// rebuild pipeline, setting current stream if needed
	gboolean              (*reset)      (LomoPlayer *self, GError **error),
	
	// query current stream
	const LomoStream      (*get_stream) (LomoPlayer *self),

	// get/state [LOMO_STATE_PLAY,LOMO_STATE_PAUSE,LOMO_STATE_STOP]
	LomoStateChangeReturn (*set_state)  (LomoPlayer *self, LomoState state, GError **error),
	LomoState             (*get_state)  (LomoPlayer *self),

	// tell/seek position of the stream
	gint64                (*tell)       (LomoPlayer *self, LomoFormat format),
	gboolean              (*seek)       (LomoPlayer *self, LomoFormat format, gint64 val),

	// query length of the stream
	gint64                (*length)     (LomoPlayer *self, LomoFormat format),

	// get/set volume
	gboolean              (*set_volume) (LomoPlayer *self, gint val),
	gint                  (*get_volume) (LomoPlayer *self),

	// get/set mute
	gboolean              (*set_mute)   (LomoPlayer *self, gboolean mute),
	gboolean              (*get_mute)   (LomoPlayer *self),

	/*
	 * Playlist functions
	 */
	// Add/delete
	gint              (*add_multi_at_pos) (LomoPlayer *self, GList *streams, gint pos),
	gboolean          (*del)              (LomoPlayer *self, gint pos),


	// Navigate/query playlist
	const GList*      (*get_playlist)     (LomoPlayer *self),
	const LomoStream* (*get_nth)          (LomoPlayer *self, gint pos),
	gint              (*get_prev)         (LomoPlayer *self),
	gint              (*get_next)         (LomoPlayer *self),
	gboolean          (*go_nth)           (LomoPlayer *self, gint pos),
	gint              (*get_current)      (LomoPlayer *self), 
	gint              (*get_total)        (LomoPlayer *self),
	void              (*clear)            (LomoPlayer *self),
	void              (*randomize)        (LomoPlayer *self)

	// get/set random/repeat
	void              (*set_repeat)       (LomoPlayer *self, gboolean val),
	gboolean          (*get_repeat)       (LomoPlayer *self),
	void              (*set_random)       (LomoPlayer *self, gboolean val),
	gboolean          (*get_random)       (LomoPlayer *self),
};

#endif // _LOMO_PLAYER_VTABLE_H
