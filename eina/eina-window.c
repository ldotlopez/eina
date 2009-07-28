#include <eina/eina-window.h>

G_DEFINE_TYPE (EinaWindow, eina_window, GTK_TYPE_WINDOW)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_WINDOW, EinaWindowPrivate))

typedef struct _EinaWindowPrivate EinaWindowPrivate;

struct _EinaWindowPrivate {
	gboolean      persistant;
	GtkBox       *container;
	GtkUIManager *ui_manager;
	int dummy;
};

gboolean
window_delete_event_cb(EinaWindow *self);

static void
eina_window_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_window_parent_class)->dispose (object);
}

static void
eina_window_class_init (EinaWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaWindowPrivate));

	object_class->dispose = eina_window_dispose;
}

static void
eina_window_init (EinaWindow *self)
{
	EinaWindowPrivate *priv = GET_PRIVATE(self);

	priv->persistant = FALSE;
	priv->container = (GtkBox *) gtk_vbox_new(FALSE, 0);
	priv->ui_manager = gtk_ui_manager_new();

	gtk_box_pack_start(priv->container,
		gtk_ui_manager_get_widget(priv->ui_manager, NULL),
		FALSE, TRUE, 0
		);
	gtk_widget_show_all((GtkWidget *) priv->container);

	gtk_container_add((GtkContainer *) self, (GtkWidget *) priv->container);
	g_signal_connect(self, "delete-event", (GCallback) window_delete_event_cb, NULL);
}

EinaWindow*
eina_window_new (void)
{
	EinaWindow *self = g_object_new (EINA_TYPE_WINDOW, NULL);
	return self;
}

void
eina_window_set_persistant(EinaWindow *self, gboolean persistant)
{
	GET_PRIVATE(self)->persistant = persistant;
}

gboolean
eina_window_get_persistant(EinaWindow *self)
{
	return GET_PRIVATE(self)->persistant;
}

void
eina_window_add_widget(EinaWindow *self, GtkWidget *child, gboolean expand, gboolean fill, gint spacing)
{
	gtk_box_pack_start(GET_PRIVATE(self)->container, child, expand, fill, spacing);
}

void
eina_window_remove_widget(EinaWindow *self, GtkWidget *child)
{
	gtk_container_remove((GtkContainer *) GET_PRIVATE(self)->container, child);
}

GtkUIManager *
eina_window_get_ui_manager(EinaWindow *self)
{
	return GET_PRIVATE(self)->ui_manager;
}

gboolean
window_delete_event_cb(EinaWindow *self)
{
	return !GET_PRIVATE(self)->persistant;
}
