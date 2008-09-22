#ifndef __LOMO_PLAYER_PRIV_H__
#define __LOMO_PLAYER_PRIV_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	PLAY,
	PAUSE,
	STOP,
	SEEK,
	VOLUME,
	MUTE,
	ADD,
	DEL,
	CHANGE,
	CLEAR,
	REPEAT,
	RANDOM,
	EOS,
	ERROR,
	TAG,
	ALL_TAGS,

	LAST_SIGNAL
} LomoPlayerSignalType;

extern guint lomo_player_signals[LAST_SIGNAL];

G_END_DECLS

#endif
