/* eina-player-cover.c */

#include "eina-player-cover.h"

G_DEFINE_TYPE (EinaPlayerCover, eina_player_cover, GTK_TYPE_IMAGE)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_PLAYER_TYPE_COVE, EinaPlayerCoverPrivate))

typedef struct _EinaPlayerCoverPrivate EinaPlayerCoverPrivate;

struct _EinaPlayerCoverPrivate {
	gpointer aaa;
};

static void
eina_player_cover_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_cover_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_cover_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (eina_player_cover_parent_class)->dispose)
		G_OBJECT_CLASS (eina_player_cover_parent_class)->dispose (object);
}

static void
eina_player_cover_class_init (EinaPlayerCoverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPlayerCoverPrivate));

	object_class->get_property = eina_player_cover_get_property;
	object_class->set_property = eina_player_cover_set_property;
	object_class->dispose = eina_player_cover_dispose;
}

static void
eina_player_cover_init (EinaPlayerCover *self)
{
}

EinaPlayerCover*
eina_player_cover_new (void)
{
	return g_object_new (EINA_PLAYER_TYPE_COVE, NULL);
}

