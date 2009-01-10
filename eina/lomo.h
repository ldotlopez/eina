#ifndef _LOMO_H
#define _LOMO_H

#include <gel/gel.h>
#include <lomo/player.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define GEL_HUB_GET_LOMO(hub)    ((LomoPlayer *) gel_hub_shared_get(hub,"lomo"))
#define EINA_BASE_GET_LOMO(base) GEL_HUB_GET_LOMO(((EinaBase *)base)->hub)

#define GEL_APP_GET_LOMO(app)  ((LomoPlayer *) gel_app_shared_get(app,"lomo"))
#define EINA_OBJ_GET_LOMO(obj) GEL_APP_GET_LOMO(eina_obj_get_app(obj))

G_END_DECLS

#endif // _LOMO_H

