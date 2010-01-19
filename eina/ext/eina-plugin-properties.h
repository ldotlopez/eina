/*
 * eina/ext/eina-plugin-properties.h
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

#ifndef _EINA_PLUGIN_PROPERTIES
#define _EINA_PLUGIN_PROPERTIES

#include <gel/gel.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLUGIN_PROPERTIES eina_plugin_properties_get_type()

#define EINA_PLUGIN_PROPERTIES(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLUGIN_PROPERTIES, EinaPluginProperties))

#define EINA_PLUGIN_PROPERTIES_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PLUGIN_PROPERTIES, EinaPluginPropertiesClass))

#define EINA_IS_PLUGIN_PROPERTIES(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLUGIN_PROPERTIES))

#define EINA_IS_PLUGIN_PROPERTIES_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PLUGIN_PROPERTIES))

#define EINA_PLUGIN_PROPERTIES_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PLUGIN_PROPERTIES, EinaPluginPropertiesClass))

typedef struct {
	GtkDialog parent;
} EinaPluginProperties;

typedef struct {
	GtkDialogClass parent_class;
} EinaPluginPropertiesClass;

GType eina_plugin_properties_get_type (void);

EinaPluginProperties* eina_plugin_properties_new (GelPlugin *);

const GelPlugin*
eina_plugin_properties_get_plugin(EinaPluginProperties *self);

G_END_DECLS

#endif /* _EINA_PLUGIN_PROPERTIES */
