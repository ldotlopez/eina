#include "eina-plugin-dialog.h"

G_DEFINE_TYPE (EinaPluginDialog, eina_plugin_dialog, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogPrivate))

typedef struct _EinaPluginDialogPrivate EinaPluginDialogPrivate;

struct _EinaPluginDialogPrivate {
	gpointer dummy;
};

static void
eina_plugin_dialog_get_property (GObject *object, guint property_id,
			                        GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_dialog_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
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
}

static void
eina_plugin_dialog_init (EinaPluginDialog *self)
{
}

EinaPluginDialog*
eina_plugin_dialog_new (void)
{
	return g_object_new (EINA_TYPE_PLUGIN_DIALOG, NULL);
}

