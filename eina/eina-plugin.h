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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gel/gel-ui.h>
#include <eina/ext/eina-application.h>
#include <eina/ext/eina-stock.h>
#include <eina/ext/eina-fs.h>

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

#define eina_plugin_get_application(plugin) EINA_APPLICATION(gel_plugin_get_application(plugin))

#endif
