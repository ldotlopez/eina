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
 * You should have received a copy of the GNU General Public
 * License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "eina-player.h"

G_DEFINE_TYPE (EinaPlayer, eina_player, GTK_TYPE_BOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLAYER, EinaPlayerPrivate))

typedef struct _EinaPlayerPrivate EinaPlayerPrivate;

struct _EinaPlayerPrivate {
	int dummy;
};

static void
eina_player_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_player_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
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
}

static void
eina_player_init (EinaPlayer *self)
{
	gtk_box_pack_start (GTK_BOX(self), (GtkWidget *) gtk_label_new("Test"), TRUE, TRUE, 0);
}

GtkWidget *
eina_player_new (void)
{
	return g_object_new (EINA_TYPE_PLAYER, NULL);
}

