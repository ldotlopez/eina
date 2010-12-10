/* eina-application.h */

#ifndef _EINA_APPLICATION
#define _EINA_APPLICATION

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_APPLICATION eina_application_get_type()
#define EINA_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_APPLICATION, EinaApplication))
#define EINA_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_APPLICATION, EinaApplicationClass))
#define EINA_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_APPLICATION))
#define EINA_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_APPLICATION))
#define EINA_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_APPLICATION, EinaApplicationClass))

typedef struct _EinaApplicationPrivate EinaApplicationPrivate;
typedef struct {
	GtkApplication parent;
	EinaApplicationPrivate *priv;
} EinaApplication;

typedef struct {
	GtkApplicationClass parent_class;
	gboolean (*action_activate) (EinaApplication *application, GtkAction *action);
} EinaApplicationClass;

GType eina_application_get_type (void);

EinaApplication* eina_application_new (const gchar *application_id);

gint*     eina_application_get_argc(EinaApplication *application);
gchar***  eina_application_get_argv(EinaApplication *application);

gboolean eina_application_set_interface(EinaApplication *application, const gchar *name, gpointer interface);
gpointer eina_application_get_interface(EinaApplication *application, const gchar *name);
gpointer eina_application_steal_interface(EinaApplication *application, const gchar *name);

GtkWindow*    eina_application_get_window             (EinaApplication *self);
GtkUIManager* eina_application_get_window_ui_manager  (EinaApplication *self);
GtkVBox*      eina_application_get_window_content_area(EinaApplication *self);

GSettings*    eina_application_get_settings(EinaApplication *self, const gchar *domain);

G_END_DECLS

#endif /* _EINA_APPLICATION */
