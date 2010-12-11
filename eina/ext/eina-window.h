#ifndef _EINA_WINDOW
#define _EINA_WINDOW

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_WINDOW eina_window_get_type()

#define EINA_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_WINDOW, EinaWindow))
#define EINA_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_WINDOW, EinaWindowClass))
#define EINA_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_WINDOW))
#define EINA_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_WINDOW))
#define EINA_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_WINDOW, EinaWindowClass))

typedef struct _EinaWindowPrivate EinaWindowPrivate;
typedef struct {
	GtkWindow parent;
	EinaWindowPrivate *priv;
} EinaWindow;

typedef struct {
	GtkWindowClass parent_class;
	gboolean (*action_activate) (EinaWindow *window, GtkAction *action);
} EinaWindowClass;

GType eina_window_get_type (void);

EinaWindow*   eina_window_new (void);

GtkUIManager* eina_window_get_ui_manager  (EinaWindow *self);

G_END_DECLS

#endif /* _EINA_WINDOW */


