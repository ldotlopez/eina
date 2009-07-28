/*
 * eina/ext/eina-volume.c
 *
 * Copyright (C) 2004-2009 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gel/gel.h>
#include <eina/ext/eina-volume.h>

G_DEFINE_TYPE (EinaVolume, eina_volume, GTK_TYPE_VOLUME_BUTTON)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_VOLUME, EinaVolumePrivate))

// #define EINA_VOLUME_DYNAMIC_UPDATE

typedef struct _EinaVolumePrivate EinaVolumePrivate;

struct _EinaVolumePrivate
{
	LomoPlayer *lomo;
};

enum {
	EINA_VOLUME_LOMO_PLAYER_PROPERTY = 1
};

#ifdef EINA_VOLUME_DYNAMIC_UPDATE
static gboolean
eina_volume_update(EinaVolume *self);
#endif

static void
on_eina_volume_value_changed (GtkWidget *w, gdouble value, EinaVolume *self);

static void
eina_volume_get_property(GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	EinaVolume *self = EINA_VOLUME(object);

	switch (property_id) {
	case EINA_VOLUME_LOMO_PLAYER_PROPERTY:
		g_value_set_object(value, (gpointer) eina_volume_get_lomo_player(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_volume_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaVolume *self = EINA_VOLUME(object);

	switch (property_id) {
	case EINA_VOLUME_LOMO_PLAYER_PROPERTY:
		eina_volume_set_lomo_player(self, LOMO_PLAYER(g_value_get_object(value)));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_volume_dispose (GObject *object)
{
	EinaVolume *self = EINA_VOLUME(object);
	EinaVolumePrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->lomo, NULL, g_object_unref);

	if (G_OBJECT_CLASS (eina_volume_parent_class)->dispose)
		G_OBJECT_CLASS (eina_volume_parent_class)->dispose (object);
}

static void
eina_volume_class_init (EinaVolumeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaVolumePrivate));

	object_class->dispose = eina_volume_dispose;
	object_class->get_property = eina_volume_get_property;
	object_class->set_property = eina_volume_set_property;

	g_object_class_install_property(object_class, EINA_VOLUME_LOMO_PLAYER_PROPERTY,
		g_param_spec_object("lomo-player", "Lomo player", "Lomo Player to control/watch",
		LOMO_TYPE_PLAYER, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
eina_volume_init (EinaVolume *self)
{
	g_signal_connect(self, "value-changed",
		G_CALLBACK(on_eina_volume_value_changed), self);
#ifdef EINA_VOLUME_DYNAMIC_UPDATE
	eina_volume_update(self);
	g_timeout_add(900, (GSourceFunc) eina_volume_update, (gpointer) self);
#endif
}

EinaVolume*
eina_volume_new (void)
{
	return g_object_new (EINA_TYPE_VOLUME, NULL);
}

void
eina_volume_set_lomo_player(EinaVolume *self, LomoPlayer *lomo)
{
	struct _EinaVolumePrivate *priv = GET_PRIVATE(self);

	if (!LOMO_IS_PLAYER(lomo))
		return;

	if (priv->lomo)
		g_object_unref(priv->lomo);

	g_object_ref(lomo);
	priv->lomo = lomo;

	gtk_scale_button_set_value(GTK_SCALE_BUTTON(self),
		(gdouble) lomo_player_get_volume(lomo) / 100);
#ifdef EINA_VOLUME_DYNAMIC_UPDATE
	eina_volume_update(self);
#endif
}

LomoPlayer*
eina_volume_get_lomo_player(EinaVolume *self)
{
	struct _EinaVolumePrivate *priv = GET_PRIVATE(self);
	return priv->lomo;
}

#ifdef EINA_VOLUME_DYNAMIC_UPDATE
static gboolean
eina_volume_update(EinaVolume *self)
{
	struct _EinaVolumePrivate *priv = GET_PRIVATE(self);
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
on_eina_volume_value_changed (GtkWidget *w, gdouble value, EinaVolume *self)
{
	EinaVolumePrivate *priv = GET_PRIVATE(self);

	if (priv->lomo)
		lomo_player_set_volume(priv->lomo, (value * 100) / 1);
}

