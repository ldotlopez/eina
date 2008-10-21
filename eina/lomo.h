#ifndef _LOMO_H
#define _LOMO_H

#include <gel/gel.h>

#define GEL_HUB_GET_LOMO(hub)    gel_hub_shared_get(hub,"lomo")
#define EINA_BASE_GET_LOMO(base) GEL_HUB_GET_LOMO(((EinaBase *)base)->hub)

#endif // _LOMO_H

