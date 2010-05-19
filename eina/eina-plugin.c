/*
 * eina/eina-plugin.c
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

#define GEL_DOMAIN "Eina::Plugin"

#include <config.h>
#include <string.h> // strcmp
#include <gmodule.h>
#include <gel/gel.h>
#include <eina/eina-plugin.h>

// Internal access only: player, dock, preferences
#include <eina/player.h>
#include <eina/dock.h>
#include <eina/preferences.h>

// --
// Utilities for plugins
// --
gchar *
eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource)
{
	gchar *dirname, *ret;
	dirname = g_path_get_dirname(gel_plugin_get_pathname(plugin));

	ret = g_build_filename(dirname, resource, NULL);
	g_free(dirname);

	return ret;
}

gchar *
eina_plugin_build_userdir_path (EinaPlugin *self, gchar *path)
{
	gchar *dirname = g_path_get_dirname(gel_plugin_get_pathname(self));
	gchar *bname   = g_path_get_basename(dirname);
	gchar *ret     = g_build_filename(g_get_home_dir(), "." PACKAGE, bname, path, NULL);
	g_free(bname);
	g_free(dirname);
	
	return ret;
}


// --
// Art handling
Art*
eina_plugin_get_art(EinaPlugin *plugin)
{
	GelApp *app;

	if ((app = gel_plugin_get_app(plugin)) == NULL)
		return NULL;

	return GEL_APP_GET_ART(app);
}

ArtBackend*
eina_plugin_add_art_backend(EinaPlugin *plugin, gchar *id,
	ArtFunc search, ArtFunc cancel, gpointer data)
{
	Art *art = eina_plugin_get_art(plugin);
	g_return_val_if_fail(art != NULL, NULL);
	
	return art_add_backend(art, id, search, cancel, data);
}

void
eina_plugin_remove_art_backend(EinaPlugin *plugin, ArtBackend *backend)
{
	Art *art = eina_plugin_get_art(plugin);
	g_return_if_fail(art != NULL);

	art_remove_backend(art, backend);
}

