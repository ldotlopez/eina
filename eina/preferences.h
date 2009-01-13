#ifndef _PREFERENCES2_H
#define _PREFERENCES2_H

#include <gel/gel.h>
#include <eina/eina-obj.h>
#include <eina/eina-preferences-dialog.h>

G_BEGIN_DECLS

#define GEL_APP_GET_PREFERENCES(app)  ((EinaPreferencesDialog *) gel_app_shared_get(app, "preferences"))
#define EINA_OBJ_GET_PREFERENCES(obj) GEL_APP_GET_PREFERENCES(eina_obj_get_app(obj))

G_END_DECLS

#endif // _PREFERENCES_H

