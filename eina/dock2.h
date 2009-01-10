#ifndef _EINA_DOCK
#define _EINA_DOCK

#include <gtk/gtk.h>
#include <gel/gel.h>
#include <eina/eina-obj.h>

G_BEGIN_DECLS

#define EINA_DOCK(p)           ((EinaDock *) p)
#define GEL_APP_GET_DOCK(app)  EINA_DOCK(gel_app_shared_get(app, "dock"))
#define EINA_OBJ_GET_DOCK(obj) GEL_APP_GET_DOCK(eina_obj_get_app(obj))

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
