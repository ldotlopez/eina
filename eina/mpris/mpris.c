/*
 * eina/mpris/mpris.c
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

#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-mpris-player.h"
#if HAVE_INDICATE
#include <libindicate/server.h>
#endif

typedef struct {
	EinaMprisPlayer *emp;
	#if HAVE_INDICATE
	IndicateServer  *is;
	#endif
} MprisPlugin;

gboolean
mpris_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = g_new(MprisPlugin, 1);

	_plugin->emp = eina_mpris_player_new(app, PACKAGE);

	#if HAVE_INDICATE
	_plugin->is = indicate_server_ref_default();
	indicate_server_set_type(_plugin->is, "music."PACKAGE);

	gchar *uc_package = g_ascii_strup(PACKAGE, -1);
	gchar *env_package_name = g_strconcat(uc_package, "_DESKTOP_FILE", NULL);
	const gchar *env_desktop_file = g_getenv(env_package_name);
	if (env_desktop_file)
	{
		g_free(uc_package);
		g_free(env_package_name);
		g_warning("Set indicate desktop file to: '%s'", env_desktop_file);
		indicate_server_set_desktop_file(_plugin->is, env_desktop_file);
	}
	else
	{
		gchar *tmp = g_strconcat(PACKAGE, ".desktop", NULL);
		gchar *desktop_file = g_build_filename(PACKAGE_PREFIX, "share", "applications", tmp, NULL);
		g_warning("Set indicate desktop file to: '%s'", desktop_file);
		indicate_server_set_desktop_file(_plugin->is, desktop_file);
		g_free(tmp);
		g_free(desktop_file);
	}
	indicate_server_show(_plugin->is);
	#endif

	gel_plugin_set_data(plugin, _plugin);

	return TRUE;
}

gboolean
mpris_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	MprisPlugin *_plugin = gel_plugin_steal_data(plugin);
	gel_free_and_invalidate(_plugin->emp, NULL, g_object_unref);
	#if HAVE_INDICATE
	gel_free_and_invalidate(_plugin->is, NULL, g_object_unref);
	#endif
	g_free(_plugin);
	return TRUE;
}

