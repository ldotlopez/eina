#ifndef _EINA_WINDOW
#define _EINA_WINDOW

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_WINDOW eina_window_get_type()

#define EINA_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_WINDOW, EinaWindow))

#define EINA_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_WINDOW, EinaWindowClass))

#define EINA_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_WINDOW))

#define EINA_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_WINDOW))

#define EINA_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_WINDOW, EinaWindowClass))

typedef struct {
	GtkWindow parent;
} EinaWindow;

typedef struct {
	GtkWindowClass parent_class;
} EinaWindowClass;

GType eina_window_get_type (void);

EinaWindow* eina_window_new (void);

void
eina_window_set_persistant(EinaWindow *self, gboolean persistant);
gboolean
eina_window_get_persistant(EinaWindow *self);

void
eina_window_add_widget(EinaWindow *self, GtkWidget *child, gboolean expand, gboolean fill, gint spacing);
void
eina_window_remove_widget(EinaWindow *self, GtkWidget *child);

GtkUIManager *
eina_window_get_ui_manager(EinaWindow *self);

#define eina_window_set_title(self,title) gtk_window_set_title((GtkWindow*) self, title)

G_END_DECLS

#endif /* _EINA_WINDOW */
