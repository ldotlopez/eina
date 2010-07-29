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

#include "eina-player.h"
#include "eina-player.ui.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (EinaPlayer, eina_player, GEL_UI_TYPE_GENERIC)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLAYER, EinaPlayerPrivate))

typedef struct _EinaPlayerPrivate EinaPlayerPrivate;

struct _EinaPlayerPrivate {
	LomoPlayer *lomo;
};

enum {
	PROP_LOMO_PLAYER = 1
};

GEL_DEFINE_WEAK_REF_CALLBACK(eina_player)

static void
action_activated_cb(GtkAction *action, EinaPlayer *self);

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
}

GtkWidget*
eina_player_new (void)
{
	return g_object_new (EINA_TYPE_PLAYER, 
	/*"xml-string", xml_ui_string,*/ NULL);
}

void
eina_player_set_lomo_player(EinaPlayer *self, LomoPlayer *lomo)
{
	g_return_if_fail(EINA_IS_PLAYER(self));
	g_return_if_fail(LOMO_IS_PLAYER(self));

	EinaPlayerPrivate *priv = GET_PRIVATE(self);
	if (priv->lomo)
	{
		g_object_weak_unref ((GObject *) priv->lomo, eina_player_weak_ref_cb, NULL);
		g_object_unref      ((GObject *) priv->lomo);
	}

	priv->lomo = lomo;
	g_object_weak_ref ((GObject *) priv->lomo, eina_player_weak_ref_cb, NULL);
	g_object_ref      ((GObject *) priv->lomo);
}

static void
action_activated_cb(GtkAction *action, EinaPlayer *self)
{
	
}
