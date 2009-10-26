/* eina-plugin-properties.c */

#include "eina-plugin-properties.h"
#include "eina-plugin-properties-ui.h"

G_DEFINE_TYPE (EinaPluginProperties, eina_plugin_properties, GTK_TYPE_VBOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLUGIN_PROPERTIES, EinaPluginPropertiesPrivate))

typedef struct _EinaPluginPropertiesPrivate EinaPluginPropertiesPrivate;

struct _EinaPluginPropertiesPrivate {
		int dummy;
};

static void
eina_plugin_properties_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_properties_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_properties_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_plugin_properties_parent_class)->dispose (object);
}

static void
eina_plugin_properties_class_init (EinaPluginPropertiesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPluginPropertiesPrivate));

	object_class->get_property = eina_plugin_properties_get_property;
	object_class->set_property = eina_plugin_properties_set_property;
	object_class->dispose = eina_plugin_properties_dispose;
}

static void
eina_plugin_properties_init (EinaPluginProperties *self)
{
}

EinaPluginProperties*
eina_plugin_properties_new (void)
{
	return g_object_new (EINA_TYPE_PLUGIN_PROPERTIES, NULL);
}

