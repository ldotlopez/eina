#ifndef _EINA_DOCK
#define _EINA_DOCK

#include <gel/gel.h>

#define EINA_DOCK(p)             ((EinaDock *) p)
#define GEL_HUB_GET_DOCK(hub)    EINA_DOCK(gel_hub_shared_get(hub, "dock"))
#define EINA_BASE_GET_DOCK(base) GEL_HUB_GET_DOCK(EINA_BASE(base)->hub)

G_BEGIN_DECLS

typedef struct _EinaDock EinaDock;

GtkWidget*
eina_dock_get_widget(GtkWidget *owner);

gboolean
eina_dock_add_widget(EinaDock *self, gchar *id, GtkWidget *label, GtkWidget *widget);

gboolean
eina_dock_remove_widget(EinaDock *self, gchar *id);

gboolean
eina_dock_switch_widget(EinaDock *iface, gchar *id);

G_END_DECLS

#endif // _EINA_DOCK
