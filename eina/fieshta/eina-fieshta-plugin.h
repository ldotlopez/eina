/*
 * plugins/fieshta/eina-fieshta-plugin.h
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

#ifndef __EINA_FIESHTA_PLUGIN_H__
#define __EINA_FIESHTA_PLUGIN_H__

#include <eina/ext/eina-extension.h>

G_BEGIN_DECLS

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_FIESHTA_PLUGIN         (eina_fieshta_plugin_get_type ())
#define EINA_FIESHTA_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPlugin))
#define EINA_FIESHTA_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPlugin))
#define EINA_IS_FIESHTA_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_FIESHTA_PLUGIN))
#define EINA_IS_FIESHTA_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_FIESHTA_PLUGIN))
#define EINA_FIESHTA_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPluginClass))

EINA_DEFINE_EXTENSION_HEADERS(EinaFieshtaPlugin, eina_fieshta_plugin)

/**
 * EinaApplication accessors
 */
typedef struct _EinaFieshta EinaFieshta;

EinaFieshta *eina_application_get_fieshta(EinaApplication *application);

void eina_application_set_fieshta_mode(EinaApplication *application, gboolean mode);
void eina_fieshta_set_mode            (EinaFieshta *self, gboolean mode);

G_END_DECLS

#endif // __EINA_FIESHTA_PLUGIN_H__

