#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <gel/gel.h>
#include <eina/eina-obj.h>
#include <eina/class-conf-file.h>

#define EINA_SETTINGS(p)           ((EinaConf *) p)
#define GEL_APP_GET_SETTINGS(app)  EINA_SETTINGS(gel_app_shared_get(app,"settings"))
#define EINA_OBJ_GET_SETTINGS(obj) GEL_APP_GET_SETTINGS(eina_obj_get_app(obj))

#endif

