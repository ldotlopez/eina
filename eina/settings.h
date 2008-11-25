#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <gel/gel.h>
#include <eina/base.h>
#include <eina/class-conf-file.h>

#define EINA_SETTINGS(p)             ((EinaConf *) p)
#define GEL_HUB_GET_SETTINGS(hub)    EINA_SETTINGS(gel_hub_shared_get(hub,"settings"))
#define EINA_BASE_GET_SETTINGS(base) GEL_HUB_GET_SETTINGS(EINA_BASE(base)->hub)

#endif // _SETTINGS_H

