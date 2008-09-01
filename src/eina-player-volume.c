#define GEL_DOMAIN "Eina::Player::Volume"

#include "eina-player-volume.h"
#include <gel/gel.h>

G_DEFINE_TYPE (EinaPlayerVolume, eina_player_volume, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_PLAYER_TYPE_VOLUME, EinaPlayerVolumePrivate))

typedef struct _EinaPlayerVolumePrivate EinaPlayerVolumePrivate;

struct _EinaPlayerVolumePrivate
{
	LomoPlayer *lomo;
	GtkWidget  *widget;
};

static void
on_eina_player_volume_value_changed (GtkWidget *w, gdouble value, EinaPlayerVolume *self);

static void
eina_player_volume_dispose (GObject *object)
{
	EinaPlayerVolume *self = EINA_PLAYER_VOLUME(object);
	EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);

	if (priv->widget && G_IS_OBJECT(priv->widget))
	{
		g_object_unref(priv->widget);
		priv->widget = NULL;
	}

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
	EinaPlayerVolumePrivate *priv =  GET_PRIVATE(self);
	static const gchar *icons[] = {
		"audio-volume-mute",
		"audio-volume-high",
		"audio-volume-low",
		"audio-volume-medium",
		NULL
	};

	priv->widget = gtk_volume_button_new();
	gtk_scale_button_set_icons(GTK_SCALE_BUTTON(priv->widget), icons);
	g_signal_connect(priv->widget, "value-changed",
		G_CALLBACK(on_eina_player_volume_value_changed), self);
	g_object_ref(priv->widget);
}

EinaPlayerVolume*
eina_player_volume_new (LomoPlayer *lomo)
{
	EinaPlayerVolume *self;
	EinaPlayerVolumePrivate *priv;

	self = g_object_new (EINA_PLAYER_TYPE_VOLUME, NULL);
	priv = GET_PRIVATE(self);

	priv->lomo = lomo;
	if ((priv->lomo == NULL)) {
		g_free(self);
		gel_error("Cannot get reference LomoPlayer");
		return NULL;
	}

	gtk_scale_button_set_value(GTK_SCALE_BUTTON(priv->widget),
		((gdouble) lomo_player_get_volume(lomo)) / 100);

	return self;
}

GtkWidget*
eina_player_volume_get_widget(EinaPlayerVolume *self)
{
	return GET_PRIVATE(self)->widget;
}

static void
on_eina_player_volume_value_changed (GtkWidget *w, gdouble value, EinaPlayerVolume *self)
{
	EinaPlayerVolumePrivate *priv = GET_PRIVATE(self);

    value = (value * 100) / 1;
	lomo_player_set_volume(priv->lomo, value);
}
