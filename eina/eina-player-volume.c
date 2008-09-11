#define GEL_DOMAIN "Eina::Player::Volume"

#include "eina-player-volume.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaPlayerVolume, eina_player_volume, GTK_TYPE_VOLUME_BUTTON)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_PLAYER_TYPE_VOLUME, EinaPlayerVolumePrivate))

// #define EINA_PLAYER_VOLUME_DYNAMIC_UPDATE

typedef struct _EinaPlayerVolumePrivate EinaPlayerVolumePrivate;

struct _EinaPlayerVolumePrivate
{
	LomoPlayer *lomo;
};

enum {
	EINA_PLAYER_VOLUME_LOMO_PLAYER_PROPERTY = 1
};

#ifdef EINA_PLAYER_VOLUME_DYNAMIC_UPDATE
static gboolean
eina_player_volume_update(EinaPlayerVolume *self);a
#endif

static void
on_eina_player_volume_value_changed (GtkWidget *w, gdouble value, EinaPlayerVolume *self);

static void
eina_player_volume_get_property(GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaPlayerVolume *self = EINA_PLAYER_VOLUME(object);

	switch (property_id) {
	case EINA_PLAYER_VOLUME_LOMO_PLAYER_PROPERTY:
		g_value_set_object(value, (gpointer) eina_player_volume_get_lomo_player(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_volume_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaPlayerVolume *self = EINA_PLAYER_VOLUME(object);

	switch (property_id) {
	case EINA_PLAYER_VOLUME_LOMO_PLAYER_PROPERTY:
		eina_player_volume_set_lomo_player(self, LOMO_PLAYER(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_volume_dispose (GObject *object)
{
	EinaPlayerVolume *self = EINA_PLAYER_VOLUME(object);
	EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);

	if (G_OBJECT_CLASS (eina_player_volume_parent_class)->dispose)
		G_OBJECT_CLASS (eina_player_volume_parent_class)->dispose (object);
}

static void
eina_player_volume_class_init (EinaPlayerVolumeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GdkPixbuf *pb;
	GError *err = NULL;
	gint i;
	gchar *tmp;
	static const gchar *icons[] = {
		"audio-volume-mute",
		"audio-volume-high",
		"audio-volume-low",
		"audio-volume-medium",
		NULL
	};
	static const gchar *filenames[] = {
		"audio-volume-mute.png",
		"audio-volume-high.png",
		"audio-volume-low.png",
		"audio-volume-medium.png",
		NULL
	};

	g_type_class_add_private (klass, sizeof (EinaPlayerVolumePrivate));

	object_class->dispose = eina_player_volume_dispose;
	object_class->get_property = eina_player_volume_get_property;
	object_class->set_property = eina_player_volume_set_property;

	g_object_class_install_property(object_class, EINA_PLAYER_VOLUME_LOMO_PLAYER_PROPERTY,
		g_param_spec_object("lomo-player", "Lomo player", "Lomo Player to control/watch",
		LOMO_TYPE_PLAYER, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	for (i = 0; icons[i] != NULL; i++) {
		if ((tmp = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, (gchar *) filenames[i])) == NULL) {
			gel_warn("Cannot find %s pixmap", filenames[i]);
			continue;
		}

		if ((pb = gdk_pixbuf_new_from_file(tmp, &err)) == NULL) {
			gel_warn("Cannot load %s into pixbuf: %s", tmp, err->message);
			g_error_free(err);
			g_free(tmp);
			continue;
		}
		g_free(tmp);
		gtk_icon_theme_add_builtin_icon(icons[i], 22, pb);
	}
}

static void
eina_player_volume_init (EinaPlayerVolume *self)
{
	static const gchar *icons[] = {
		"audio-volume-mute",
		"audio-volume-high",
		"audio-volume-low",
		"audio-volume-medium",
		NULL
	};

	gtk_scale_button_set_icons(GTK_SCALE_BUTTON(self), icons);
	g_signal_connect(self, "value-changed",
		G_CALLBACK(on_eina_player_volume_value_changed), self);
#ifdef EINA_PLAYER_VOLUME_DYNAMIC_UPDATE
	eina_player_volume_update(self);
	g_timeout_add(900, (GSourceFunc) eina_player_volume_update, (gpointer) self);
#endif
}

EinaPlayerVolume*
eina_player_volume_new (void)
{
	return g_object_new (EINA_PLAYER_TYPE_VOLUME, NULL);
}

void
eina_player_volume_set_lomo_player(EinaPlayerVolume *self, LomoPlayer *lomo)
{
	struct _EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);

	if (!LOMO_IS_PLAYER(lomo))
		return;

	if (priv->lomo)
		g_object_unref(priv->lomo);

	g_object_ref(lomo);
	priv->lomo = lomo;

	gtk_scale_button_set_value(GTK_SCALE_BUTTON(self),
		(gdouble) lomo_player_get_volume(lomo) / 100);
#ifdef EINA_PLAYER_VOLUME_DYNAMIC_UPDATE
	eina_player_volume_update(self);
#endif
}

LomoPlayer*
eina_player_volume_get_lomo_player(EinaPlayerVolume *self)
{
	struct _EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

#ifdef EINA_PLAYER_VOLUME_DYNAMIC_UPDATE
static gboolean
eina_player_volume_update(EinaPlayerVolume *self)
{
	struct _EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);
	gint value;
	gdouble v;

	if (priv->lomo != NULL)
	{
		value = lomo_player_get_volume(priv->lomo);
		v = ((gdouble) value) / 100;
		gtk_scale_button_set_value(GTK_SCALE_BUTTON(self), v);
	}
	
	return TRUE;
}
#endif

static void
on_eina_player_volume_value_changed (GtkWidget *w, gdouble value, EinaPlayerVolume *self)
{
	EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);

	if (priv->lomo)
		lomo_player_set_volume(priv->lomo, (value * 100) / 1);
}

