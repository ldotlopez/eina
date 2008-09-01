#include "class-base.h"

G_DEFINE_TYPE (EinaBase, eina_base, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_BASE, EinaBasePrivate))

typedef struct _EinaBasePrivate EinaBasePrivate;

struct _EinaBasePrivate {
};

static void
eina_base_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
eina_base_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
eina_base_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (eina_base_parent_class)->dispose)
    G_OBJECT_CLASS (eina_base_parent_class)->dispose (object);
}

static void
eina_base_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (eina_base_parent_class)->finalize)
    G_OBJECT_CLASS (eina_base_parent_class)->finalize (object);
}

static void
eina_base_class_init (EinaBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (EinaBasePrivate));

  object_class->get_property = eina_base_get_property;
  object_class->set_property = eina_base_set_property;
  object_class->dispose = eina_base_dispose;
  object_class->finalize = eina_base_finalize;
}

static void
eina_base_init (EinaBase *self)
{
}

EinaBase*
eina_base_new (void)
{
  return g_object_new (EINA_TYPE_BASE, NULL);
}


