/*
 * eina/eina-plugin.h
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

#ifndef _EINA_PLUGIN
#define _EINA_PLUGIN

#include <config.h>

// system libs
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

// liblomo
#include <lomo/lomo-player.h>

// libgel
#include <gel/gel.h>
#include <gel/gel-ui.h>

// Modules availables: lomo, settings, art, fs (utility functions)
#include <eina/art.h>
#include <eina/dock.h>
#include <eina/fs.h>
#include <eina/lomo.h>
#include <eina/preferences.h>
#include <eina/settings.h>
#include <eina/window.h>
#include <eina/ext/curl-engine.h>
#include <eina/ext/eina-cover.h>
#include <eina/ext/eina-file-chooser-dialog.h>
#include <eina/ext/eina-file-utils.h>
#include <eina/ext/eina-preferences-dialog.h>
#include <eina/ext/eina-stock.h>

// Redefine some types and enums
#define EinaPluginInfo GelPluginInfo
#define EinaPlugin     GelPlugin
#define EINA_PLUGIN_SERIAL GEL_PLUGIN_SERIAL
#define EINA_PLUGIN_GENERIC_AUTHOR "Luis Lopez <luis.lopez@cuarentaydos.com>"
#define EINA_PLUGIN_GENERIC_URL    "http://eina.sourceforge.net/"
#define EINA_PLUGIN(p)      GEL_PLUGIN(p)

// If EINA_PLUGIN_DATA_TYPE is defined create a macro to easy access
#ifdef EINA_PLUGIN_DATA_TYPE
#define EINA_PLUGIN_DATA(p) ((EINA_PLUGIN_DATA_TYPE *) gel_plugin_get_data((GelPlugin *) p))
#endif

// Define a macro for define plugin struct easily
#define EINA_PLUGIN_INFO_SPEC(name,version,deps,author,url,short_desc,long_desc,icon) \
	G_MODULE_EXPORT EinaPluginInfo name ## _plugin_info = { \
		G_STRINGIFY(name), NULL, NULL,                \
                                                      \
		version ? version : PACKAGE_VERSION,          \
		deps,                                         \
		author ? author : EINA_PLUGIN_GENERIC_AUTHOR, \
		url    ? url    : EINA_PLUGIN_GENERIC_URL,    \
                                                      \
		short_desc, long_desc, icon                   \
	}

// --
// Art handling
// --
ArtBackend *
eina_plugin_add_art_backend(EinaPlugin *plugin, gchar *id,
	ArtFunc search, ArtFunc cancel, gpointer data);
void
eina_plugin_remove_art_backend(EinaPlugin *plugin, ArtBackend *backend);

#endif /* _EINA_PLUGIN */
