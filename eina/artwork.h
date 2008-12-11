#ifndef _ARTWORK_H
#define _ARTWORK_H

#include <eina/eina-artwork.h>

#define GEL_HUB_GET_ARTWORK(hub)    EINA_ARTWORK(gel_hub_shared_get(hub, "artwork"))
#define EINA_BASE_GET_ARTWORK(base) GEL_HUB_GET_ARTWORK(EINA_BASE(base)->hub)

#endif
