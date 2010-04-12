/*
 * plugins/clutty/clutty.c
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

#include "eina/eina-plugin.h"
#include "eina/player.h"
#include "eina-cover-clutter.h"
#define GEL_DOMAIN "Eina::Plugin::Clutty"
#define CLUTTY_VERSION "1.0.0"

enum {
	EINA_CLUTTY_NO_ERROR = 0,
	EINA_CLUTTY_ERROR_NO_COVER_WIDGET,
	EINA_CLUTTY_ERROR_INVALID_RENDERER_TYPE,
	EINA_CLUTTY_ERROR_INIT_FAIL
};

static GQuark
clutty_quark(void)
{
	static GQuark ret = 0;
	if (!ret)
		ret = g_quark_from_static_string("clutty-quark");
	return ret;
}

G_MODULE_EXPORT gboolean
clutty_init(GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaCover *cover = eina_plugin_player_get_cover_widget(plugin);
	if (!cover)
	{
		g_set_error(error, clutty_quark(), EINA_CLUTTY_ERROR_NO_COVER_WIDGET, N_("Cover widget not found"));
		return FALSE;
	}

	ClutterInitError e = clutter_init(NULL, NULL);
	if (e <= 0)
	{
		g_set_error(error, clutty_quark(), EINA_CLUTTY_ERROR_INIT_FAIL, N_("Unable to init clutter-gtk. Error code: %d"), e);
		return FALSE;
	}

	EinaCoverClutter *renderer = eina_cover_clutter_new();
	
	GtkWidget *old_renderer = (GtkWidget *) eina_cover_get_renderer(cover);
	g_object_set_data((GObject *) renderer, "x-old-renderer-type", GINT_TO_POINTER(old_renderer ? G_OBJECT_TYPE(old_renderer) : G_TYPE_INVALID));
	eina_cover_set_renderer(cover, (GtkWidget *) renderer);

	return TRUE;
}

G_MODULE_EXPORT gboolean
clutty_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaCover *cover = eina_plugin_player_get_cover_widget(plugin);
	if (!cover)
	{
		g_set_error(error, clutty_quark(), EINA_CLUTTY_ERROR_NO_COVER_WIDGET, N_("Cover widget not found"));
		return FALSE;
	}

	GtkWidget *renderer = eina_cover_get_renderer(cover);
	if (!EINA_IS_COVER_CLUTTER(renderer))
	{
		g_set_error(error, clutty_quark(), EINA_CLUTTY_ERROR_INVALID_RENDERER_TYPE, N_("Current renderer is not a EinaCoverClutter"));
		return FALSE;
	}

	GType type = GPOINTER_TO_INT(g_object_get_data((GObject *) renderer, "x-old-renderer-type"));
	if (type != G_TYPE_INVALID)
		eina_cover_set_renderer(cover, g_object_new(type, NULL));

	return TRUE;
}

EINA_PLUGIN_SPEC(clutty, CLUTTY_VERSION,
	"player",
	EINA_PLUGIN_GENERIC_AUTHOR, EINA_PLUGIN_GENERIC_URL,
	N_("Cover renderer with 3D effects"),  NULL,
	"icon.png",
	clutty_init,
	clutty_fini
);

