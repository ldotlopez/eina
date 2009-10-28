#include "eina-plugin-dialog.h"
#include "eina-plugin-dialog-ui.h"

G_DEFINE_TYPE (EinaPluginDialog, eina_plugin_dialog, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogPrivate))

typedef struct _EinaPluginDialogPrivate EinaPluginDialogPrivate;

struct _EinaPluginDialogPrivate {
	GelApp *app;
};

enum {
	PROPERTY_APP = 1
};

static void
set_app(EinaPluginDialog *self, GelApp *app);
static void
eina_plugin_dialog_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	EinaPluginDialog *self = EINA_PLUGIN_DIALOG(object);

	switch (property_id) {
	case PROPERTY_APP:
		g_value_set_object(value, (GObject *) eina_plugin_dialog_get_app(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_dialog_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	EinaPluginDialog *self = EINA_PLUGIN_DIALOG(object);

	switch (property_id) {
	case PROPERTY_APP:
		set_app(self, GEL_APP(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_dialog_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_plugin_dialog_parent_class)->dispose (object);
}

static void
eina_plugin_dialog_class_init (EinaPluginDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPluginDialogPrivate));

	object_class->get_property = eina_plugin_dialog_get_property;
	object_class->set_property = eina_plugin_dialog_set_property;
	object_class->dispose = eina_plugin_dialog_dispose;

	g_object_class_install_property(object_class, PROPERTY_APP,
		g_param_spec_object("app", "Gel App", "GelApp object to handleto display",
		GEL_TYPE_APP, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));
}

static void
eina_plugin_dialog_init (EinaPluginDialog *self)
{
	gchar *objs[] = { "main-widget", NULL };
	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (gtk_builder_add_objects_from_string(builder, ui_xml, -1, objs, &error) == 0)
	{
		g_warning("%s", error->message);
		g_error_free(error);
		return;
	}

	gtk_container_add(
		(GtkContainer *) gtk_dialog_get_content_area((GtkDialog *) self),
		(GtkWidget    *)  gtk_builder_get_object(builder, "main-widget"));
	gtk_dialog_add_button((GtkDialog *) self, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	g_object_unref(builder);
}

EinaPluginDialog*
eina_plugin_dialog_new (GelApp *app)
{
	return g_object_new (EINA_TYPE_PLUGIN_DIALOG, "app", app, NULL);
}

static void
set_app(EinaPluginDialog *self, GelApp *app)
{
}

GelApp *
eina_plugin_dialog_get_app(EinaPluginDialog *self)
{
	return GET_PRIVATE(self)->app;
}
