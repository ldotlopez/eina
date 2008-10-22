#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <gel/gel.h>
#include "class-conf-file.h"

G_BEGIN_DECLS

#define GEL_HUB_GET_SETTINGS(hub)    ((EinaConf *) gel_hub_shared_get(hub,"settings"))
#define EINA_BASE_GET_SETTINGS(base) GEL_HUB_GET_SETTINGS(((EinaBase *) base)->hub)

#endif // _SETTINGS_H

