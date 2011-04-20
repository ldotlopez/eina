/*
 * eina/clutty/clutty.c
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

#include <eina/eina-plugin.h>
#include "eina-cover-clutter.h"
#include <eina/player/player.h>

static GtkWidget *prev_render = NULL;

G_MODULE_EXPORT gboolean
clutty_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
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

G_MODULE_EXPORT gboolean
clutty_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	g_return_val_if_fail(GTK_IS_WIDGET(prev_render), TRUE);

	EinaPlayer *player = eina_application_get_player(app);
	EinaCover  *cover  = eina_player_get_cover_widget(player);
	eina_cover_set_renderer(cover, prev_render);
	g_object_unref(prev_render);

	return TRUE;
}

