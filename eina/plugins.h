#ifndef _PLUGINS_H
#define _PLUGINS_H

#include <gel/gel.h>

typedef struct _EinaPlugins EinaPlugins;

#define GEL_HUB_GET_PLUGINS(hub)    gel_hub_shared_get(hub, "plugins")
#define EINA_BASE_GET_PLUGINS(base) GEL_HUB_GET_PLUGINS(HUB(base))

#endif
