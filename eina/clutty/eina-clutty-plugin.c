/*
 * eina/clutty/eina-clutty-plugin.c
 *
 * Copyright (C) 2004-2011 Eina
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

#include "eina-cover-clutter.h"
#include <eina/ext/eina-extension.h>
#include <eina/player/eina-player-plugin.h>

static GtkWidget *prev_render = NULL;

#define EINA_TYPE_CLUTTY_PLUGIN         (eina_clutty_plugin_get_type ())
#define EINA_CLUTTY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPlugin))
#define EINA_CLUTTY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPlugin))
#define EINA_IS_CLUTTY_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_CLUTTY_PLUGIN))
#define EINA_IS_CLUTTY_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_CLUTTY_PLUGIN))
#define EINA_CLUTTY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaCluttyPlugin, eina_clutty_plugin)
EINA_DEFINE_EXTENSION(EinaCluttyPlugin, eina_clutty_plugin, EINA_TYPE_CLUTTY_PLUGIN)

static gboolean
eina_clutty_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaPlayer *player = eina_application_get_player(app);
	EinaCover  *cover  = eina_player_get_cover_widget(player);
	prev_render = (GtkWidget *) eina_cover_get_renderer(cover);
	g_return_val_if_fail(GTK_IS_WIDGET(prev_render), TRUE);

	g_object_ref(prev_render);

	gtk_clutter_init(NULL, NULL);
	GtkWidget *new_cover = (GtkWidget *) eina_cover_clutter_new();
	eina_cover_set_renderer(cover, new_cover);

	return TRUE;
}

static gboolean
eina_clutty_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	g_return_val_if_fail(GTK_IS_WIDGET(prev_render), TRUE);

	EinaPlayer *player = eina_application_get_player(app);
	EinaCover  *cover  = eina_player_get_cover_widget(player);
	eina_cover_set_renderer(cover, prev_render);
	g_object_unref(prev_render);

	return TRUE;
}

