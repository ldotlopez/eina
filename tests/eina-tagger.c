#include "eina-tagger.h"

G_DEFINE_TYPE (EinaTagger, eina_tagger, GTK_TYPE_ENTRY)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_TAGGER, EinaTaggerPrivate))

typedef struct _EinaTaggerPrivate EinaTaggerPrivate;

struct _EinaTaggerPrivate {
};

static void
eina_tagger_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_tagger_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_tagger_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (eina_tagger_parent_class)->dispose)
		G_OBJECT_CLASS (eina_tagger_parent_class)->dispose (object);
}

static void
eina_tagger_class_init (EinaTaggerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaTaggerPrivate));

	object_class->get_property = eina_tagger_get_property;
	object_class->set_property = eina_tagger_set_property;
	object_class->dispose = eina_tagger_dispose;
}

static void
eina_tagger_init (EinaTagger *self)
{
}

EinaTagger*
eina_tagger_new (void)
{
	return g_object_new (EINA_TYPE_TAGGER, NULL);
}

