#include <lomo/lomo-player.h>

typedef struct _LomoStats LomoStats;

LomoStats* lomo_stats_watch(LomoPlayer *player);
void       lomo_stats_destroy(LomoStats *stats);

gint64 lomo_stats_get_time_played(LomoStats *stats);

