/*
 * eina/art/eina-art-plugin.h
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

#ifndef __EINA_ART_PLUGIN_H__
#define __EINA_ART_PLUGIN_H__

#include <eina/ext/eina-extension.h>
#include <eina/art/eina-art.h>

G_BEGIN_DECLS

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_ART_PLUGIN         (eina_art_plugin_get_type ())
#define EINA_ART_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_ART_PLUGIN, EinaArtPlugin))
#define EINA_ART_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_ART_PLUGIN, EinaArtPlugin))
#define EINA_IS_ART_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_ART_PLUGIN))
#define EINA_IS_ART_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_ART_PLUGIN))
#define EINA_ART_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_ART_PLUGIN, EinaArtPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaArtPlugin, eina_art_plugin)

/**
 * EinaApplication accessors
 */
EinaArt *eina_application_get_art(EinaApplication *application);

/**
 * API
 */
void         eina_art_plugin_init_stream(EinaArt *art, LomoStream *stream);
const gchar *eina_art_plugin_get_default_cover_path(void);
const gchar *eina_art_plugin_get_default_cover_uri (void);
const gchar *eina_art_plugin_get_loading_cover_path(void);
const gchar *eina_art_plugin_get_loading_cover_uri (void);

G_END_DECLS

#endif

