/*
 * eina/art/art.c
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

#include <eina/eina-plugin2.h>
#include "art.h"
#include "eina-art-test-backends.h"

EinaArtBackend *null_backend, *infolder_backend;

G_MODULE_EXPORT gboolean
art_plugin_init(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaArt *art = eina_art_new();

	EinaArtClass *art_class = EINA_ART_CLASS(G_OBJECT_GET_CLASS(art));

	null_backend = eina_art_class_add_backend(art_class,
                                              "null",
                                              eina_art_null_backend_search, NULL,
                                              NULL, NULL);
	infolder_backend = eina_art_class_add_backend(art_class,
                                              "infolder",
                                              eina_art_infolder_sync_backend_search, NULL,
                                              NULL, NULL);
	gel_plugin_engine_set_interface(engine, "art", art);
	return TRUE;
}

G_MODULE_EXPORT gboolean
art_plugin_fini(GelPluginEngine *engine, GelPlugin *plugin, GError **error)
{
	EinaArt *art = gel_plugin_engine_steal_interface(engine, "art");
	EinaArtClass *art_class = EINA_ART_CLASS(G_OBJECT_GET_CLASS(art));

	eina_art_class_remove_backend(art_class, null_backend);
	eina_art_class_remove_backend(art_class, infolder_backend);
	g_object_unref(art);

	return TRUE;
}
