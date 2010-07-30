/*
 * plugins/player/eina-player.c
 *
 * Copyright (C) 2004-2010 Eina
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

#include <glib/gi18n.h>
#include "eina-player.h"
#include "eina-player.ui.h"
#include "eina-seek.h"

G_DEFINE_TYPE (EinaPlayer, eina_player, GEL_UI_TYPE_GENERIC)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLAYER, EinaPlayerPrivate))

typedef struct _EinaPlayerPrivate EinaPlayerPrivate;

struct _EinaPlayerPrivate {
	LomoPlayer *lomo;
	EinaSeek   *seek;
};

enum {
	PROP_LOMO_PLAYER = 1
};

static void
player_update_state(EinaPlayer *self);
#if 0
static void
player_update_sensitive(EinaPlayer *self);
#endif

static gboolean
binding_volume_int_to_double(GBinding *binding, const GValue *src, GValue *dst, gpointer data);
static gboolean
binding_volume_double_to_int(GBinding *binding, const GValue *src, GValue *dst, gpointer data);

GEL_DEFINE_WEAK_REF_CALLBACK(eina_player)

static void
eina_player_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	case PROP_LOMO_PLAYER:
		eina_player_set_lomo_player((EinaPlayer *) object, (LomoPlayer *) g_value_get_object(value));
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_player_parent_class)->dispose (object);
}

static void
eina_player_class_init (EinaPlayerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPlayerPrivate));

	object_class->get_property = eina_player_get_property;
	object_class->set_property = eina_player_set_property;
	object_class->dispose = eina_player_dispose;

	g_object_class_install_property(object_class, PROP_LOMO_PLAYER,
		g_param_spec_object("lomo-player", "lomo-player", "lomo-player",
		LOMO_TYPE_PLAYER, G_PARAM_WRITABLE
		));
}

static void
eina_player_init (EinaPlayer *self)
{
	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);
#if 0
	g_object_set(self, "xml-string", xml_ui_string, NULL);
	GtkBuilder *builder = gel_ui_generic_get_builder((GelUIGeneric *) self);
	const gchar *actions[] = {
		"prev-action",
		"next-action",
		"play-action",
		"pause-action"
		};
	for (guint i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GtkAction *a = GTK_ACTION(gtk_builder_get_object(builder, actions[i]));
		if (!a)
			g_warning(N_("Action '%s' not found in widget"), actions[i]);
		else
			g_signal_connect(a, "activated", (GCallback) action_activated_cb, self);
	}
#endif
}

GtkWidget*
eina_player_new (void)
{
	GtkWidget *self = g_object_new (EINA_TYPE_PLAYER, 
		#if 1
		"xml-string", xml_ui_string,
		#endif
		 NULL);

	EinaPlayerPrivate *priv = GET_PRIVATE(self);

	// Seek widget
	priv->seek = eina_seek_new();
	g_object_set(priv->seek,
		"current-label",   gel_ui_generic_get_typed(self, GTK_LABEL, "time-current-label"),
		"remaining-label", gel_ui_generic_get_typed(self, GTK_LABEL, "time-remaining-label"),
		"total-label",     gel_ui_generic_get_typed(self, GTK_LABEL, "time-total-label"),
		NULL);

	gel_ui_container_replace_children(
		gel_ui_generic_get_typed(self, GTK_CONTAINER, "seek-container"),
		GTK_WIDGET(priv->seek));
	gtk_widget_show(GTK_WIDGET(priv->seek));

	return self;
}

void
eina_player_set_lomo_player(EinaPlayer *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_PLAYER(self));
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	EinaPlayerPrivate *priv = GET_PRIVATE(self);
	if (priv->lomo)
	{
		g_object_weak_unref ((GObject *) priv->lomo, eina_player_weak_ref_cb, NULL);
		g_object_unref      ((GObject *) priv->lomo);
	}

	priv->lomo = lomo;
	g_object_weak_ref ((GObject *) priv->lomo, eina_player_weak_ref_cb, NULL);
	g_object_ref      ((GObject *) priv->lomo);

	GtkBuilder *builder = gel_ui_generic_get_builder((GelUIGeneric *) self);
	g_object_bind_property_full(
		lomo, "volume",
		gtk_builder_get_object(builder, "volume-button"), "value",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
		(GBindingTransformFunc) binding_volume_int_to_double,
		(GBindingTransformFunc) binding_volume_double_to_int,
		NULL, NULL);

	g_object_set(priv->seek, "lomo-player", lomo, NULL);

	g_signal_connect_swapped(lomo, "play",   (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "pause",  (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "stop",   (GCallback) player_update_state, self);

	g_object_notify((GObject *) self, "lomo-player");
}

static void
player_update_state(EinaPlayer *self)
{
	LomoPlayer *lomo = GET_PRIVATE(self)->lomo;
	g_return_if_fail(LOMO_IS_PLAYER(lomo));

	GtkBuilder *builder = gel_ui_generic_get_builder((GelUIGeneric *) self);
	GtkActivatable *button = GTK_ACTIVATABLE(gtk_builder_get_object(builder, "play-pause-button"));
	GtkImage       *image  = GTK_IMAGE      (gtk_builder_get_object(builder, "play-pause-image"));

	const gchar *action = NULL;
	const gchar *stock = NULL;

	if (lomo_player_get_state(lomo) == LOMO_STATE_PLAY)
	{
		action = "pause-action";
		stock  = "gtk-media-pause";
	}
	else
	{
		action = "play-action";
		stock  = "gtk-media-play";
	}
	gtk_activatable_set_related_action(button, gel_ui_generic_get_typed(self, GTK_ACTION, action));
	gtk_image_set_from_stock(image, stock, GTK_ICON_SIZE_BUTTON);
}

#if 0
static void
player_update_sensitive(EinaPlayer *self)
{
	LomoPlayer *lomo = GET_PRIVATE(self)->lomo;

	if (lomo != NULL)
	{
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "play-pause-button"), (lomo_player_get_current(lomo) != -1));
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "prev-button"),       (lomo_player_get_prev(lomo) != -1));
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "next-button"),       (lomo_player_get_next(lomo) != -1));
	}
	else
	{
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "play-pause-button"), FALSE);
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "prev-button"),       FALSE);
		gtk_widget_set_sensitive(gel_ui_generic_get_typed(self, GTK_WIDGET, "next-button"),       FALSE);
	}
}
#endif

static gboolean
binding_volume_int_to_double(GBinding *binding, const GValue *src, GValue *dst, gpointer data)
{
	g_value_set_double(dst, CLAMP(g_value_get_int(src) / (gdouble) 100, 0, 1));
	return TRUE;
}

static gboolean
binding_volume_double_to_int(GBinding *binding, const GValue *src, GValue *dst, gpointer data)
{
	g_value_set_int(dst, (gint) CLAMP(g_value_get_double(src) * 100, 0, 100));
	return TRUE;
}
