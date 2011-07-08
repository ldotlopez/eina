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


#define EINA_TYPE_CLUTTY_PLUGIN         (eina_clutty_plugin_get_type ())
#define EINA_CLUTTY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPlugin))
#define EINA_CLUTTY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPlugin))
#define EINA_IS_CLUTTY_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_CLUTTY_PLUGIN))
#define EINA_IS_CLUTTY_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_CLUTTY_PLUGIN))
#define EINA_CLUTTY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPluginClass))

typedef struct {
	GtkWidget *renderer;
	GtkWidget *prev_render;
} EinaCluttyPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_CLUTTY_PLUGIN, EinaCluttyPlugin, eina_clutty_plugin)

static gboolean
eina_clutty_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaCluttyPlugin      *plugin = EINA_CLUTTY_PLUGIN(activatable);
	EinaCluttyPluginPrivate *priv = plugin->priv;

	EinaApplication *application = eina_activatable_get_application(activatable);
	EinaPlayer *player = eina_application_get_player(application);
	EinaCover  *cover  = eina_player_get_cover_widget(player);

	priv->prev_render = (GtkWidget *) eina_cover_get_renderer(cover);

	g_warn_if_fail(GTK_IS_WIDGET(priv->prev_render));
	if (GTK_IS_WIDGET(priv->prev_render))
		g_object_ref(priv->prev_render);

	gtk_clutter_init(NULL, NULL);

	priv->renderer = (GtkWidget *) eina_cover_clutter_new();
	eina_cover_set_renderer(cover, priv->renderer);
	g_object_ref(priv->renderer);

	return TRUE;
}

static gboolean
eina_clutty_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaCluttyPlugin      *plugin = EINA_CLUTTY_PLUGIN(activatable);
	EinaCluttyPluginPrivate *priv = plugin->priv;

	if (priv->prev_render)
	{
		EinaApplication *application = eina_activatable_get_application(activatable);
		EinaPlayer *player = eina_application_get_player(application);
		EinaCover  *cover  = eina_player_get_cover_widget(player);

		eina_cover_set_renderer(cover, priv->prev_render);
		g_object_unref(priv->prev_render);
		priv->prev_render = NULL;
	}
	
	g_warn_if_fail(EINA_IS_COVER_CLUTTER(priv->renderer));
	if (EINA_IS_COVER_CLUTTER(priv->renderer))
	{
		g_object_unref(priv->renderer);
		priv->renderer = NULL;
	}

	return TRUE;
}

