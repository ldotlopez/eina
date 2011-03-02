#ifndef _MPRIS_MRPIS_H
#define _MPRIS_MRPIS_H

#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>

typedef struct {
	LomoPlayer      *lomo;
	GDBusConnection *connection;
	GDBusNodeInfo   *node_info;
	guint            bus_id, root_id, player_id;
} MprisPlugin;

enum {
	MPRIS_ERROR_NOT_IMPLEMENTED = 1
};

#endif
