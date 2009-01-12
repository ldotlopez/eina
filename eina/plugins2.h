#ifndef _PLUGINS_H
#define _PLUGINS_H

#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

typedef struct _EinaPlugins EinaPlugins;

#define EINA_PLUGINS(p)           ((EinaPlugins *) p)
#define GEL_APP_GET_PLUGINS(app)  gel_app_shared_get(app, "plugins")
#define EINA_OBJ_GET_PLUGINS(obj) GEL_APP_GET_PLUGINS(eina_obj_get_app(obj))

G_END_DECLS

#endif
