#ifndef __PREFERENCES_H
#define __PREFERENCES_H

#include <gel/gel.h>
#include <eina/base.h>
#include <eina/eina-preferences-dialog.h>

G_BEGIN_DECLS

#define GEL_HUB_GET_PREFERENCES(hub)    ((EinaPreferencesDialog *) gel_hub_shared_get(hub,"preferences"))
#define EINA_BASE_GET_PREFERENCES(base) GEL_HUB_GET_PREFERENCES(((EinaBase *) base)->hub)

G_END_DECLS

#endif // _PREFERENCES_H

