/*
 * player.h
 * Public API for EinaPlayer object
 */
#ifndef __EINA_PLAYER_H__
#define __EINA_PLAYER_H__

#include "libghub/ghub.h"

typedef struct _EinaPlayer EinaPlayer;

gboolean eina_player_init(GHub *hub, gint *argc, gchar ***argv);
gboolean eina_player_exit(gpointer data);

void eina_player_switch_state_play (EinaPlayer *self);
void eina_player_switch_state_pause(EinaPlayer *self);

void eina_player_set_info(EinaPlayer *self, LomoStream *stream);

#endif
