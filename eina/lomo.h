#ifndef _LOMO_H
#define _LOMO_H

#include <lomo/player.h>
#include <gel/gel.h>

G_BEGIN_DECLS

#define GEL_HUB_GET_LOMO(hub)    ((LomoPlayer *) gel_hub_shared_get(hub,"lomo"))
#define EINA_BASE_GET_LOMO(base) GEL_HUB_GET_LOMO(((EinaBase *)base)->hub)

#if ((defined USE_GEL_APP) && (USE_GEL_APP == 1))
#define GEL_APP_GET_LOMO(app)  ((LomoPlayer *) gel_app_shared_get(app,"lomo"))
#define EINA_OBJ_GET_LOMO(obj) GEL_APP_GET_LOMO(gel_obj_get_app(obj))
#endif

G_END_DECLS

#endif // _LOMO_H

