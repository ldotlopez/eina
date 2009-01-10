#ifndef _ARTWORK2_H
#define _ARTWORK2_H

#include <eina/eina-obj.h>
#include <eina/eina-artwork.h>

#define GEL_APP_GET_ARTWORK(app)    EINA_ARTWORK(gel_app_shared_get(app, "artwork"))
#define EINA_OBJ_GET_ARTWORK(obj)   GEL_APP_GET_ARTWORK(eina_obj_get_app(obj))

#endif
