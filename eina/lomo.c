/*
 * eina/lomo.c
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

#define GEL_DOMAIN "Eina::Lomo"

#include <config.h>
#include <gmodule.h>
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <lomo/player.h>

static GQuark
lomo_quark(void)
{
	GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-lomo");
	return ret;
}

enum {
	LOMO_NO_ERROR = 0,
	LOMO_CANNOT_CREATE_ENGINE,
	LOMO_CANNOT_SET_SHARED,
	LOMO_CANNOT_DESTROY_ENGINE
};

gboolean
lomo_plugin_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	LomoPlayer *engine = NULL;

	if ((engine = lomo_player_new("audio-output", "autoaudiosink", NULL)) == NULL)
	{
		g_set_error(error, lomo_quark(), LOMO_CANNOT_CREATE_ENGINE, N_("Cannot create engine"));
		return FALSE;
	}

	if (!gel_app_shared_set(gel_plugin_get_app(plugin), "lomo", engine))
	{
		g_set_error(error, lomo_quark(), LOMO_CANNOT_SET_SHARED, N_("Cannot share engine"));
		return FALSE;
	}
	return TRUE;
}

gboolean
lomo_plugin_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	LomoPlayer *engine = LOMO_PLAYER(gel_app_shared_get(app, "lomo"));

	if ((engine == NULL) || !gel_app_shared_unregister(app, "lomo"))
	{
		g_set_error(error, lomo_quark(), LOMO_CANNOT_DESTROY_ENGINE, N_("Cannot destroy engine"));
		return FALSE;
	}
	g_object_unref(G_OBJECT(engine));

	return TRUE;
}

G_MODULE_EXPORT GelPlugin lomo_plugin = 
{
	GEL_PLUGIN_SERIAL,
	"lomo", PACKAGE_VERSION,
	N_("Build-in lomo plugin"), NULL,
	NULL, NULL, NULL,
	lomo_plugin_init,
	lomo_plugin_fini,
	NULL, NULL
};

