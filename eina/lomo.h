#ifndef _LOMO_H
#define _LOMO_H

#include <lomo/player.h>
#include <gel/gel.h>

G_BEGIN_DECLS

#define GEL_HUB_GET_LOMO(hub)    ((LomoPlayer *) gel_hub_shared_get(hub,"lomo"))
#define EINA_BASE_GET_LOMO(base) GEL_HUB_GET_LOMO(((EinaBase *)base)->hub)

G_END_DECLS

#endif // _LOMO_H

