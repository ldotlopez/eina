/*
 * eina/ext/eina-plugin-properties.c
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

#include <glib/gi18n.h>
#include <eina/ext/eina-stock.h>
#include <eina/ext/eina-plugin-properties.h>
#include <eina/ext/eina-plugin-properties-ui.h>

G_DEFINE_TYPE (EinaPluginProperties, eina_plugin_properties, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PLUGIN_PROPERTIES, EinaPluginPropertiesPrivate))

typedef struct _EinaPluginPropertiesPrivate EinaPluginPropertiesPrivate;

struct _EinaPluginPropertiesPrivate {
	GelPlugin *plugin;
	GtkImage  *icon;
	GtkLabel  *name, *short_desc, *long_desc;
	GtkLabel  *pathname, *deps, *rdeps;
};

enum {
	PROPERTY_PLUGIN = 1
};

void
set_plugin(EinaPluginProperties *self, GelPlugin *plugin);

static void
eina_plugin_properties_get_property (GObject *object, guint property_id,
		                          GValue *value, GParamSpec *pspec)
{
	EinaPluginProperties *self = EINA_PLUGIN_PROPERTIES(object);

	switch (property_id) {
	case PROPERTY_PLUGIN:
		g_value_set_pointer(value, (gpointer) eina_plugin_properties_get_plugin(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_properties_set_property (GObject *object, guint property_id,
		                          const GValue *value, GParamSpec *pspec)
{
	EinaPluginProperties *self = EINA_PLUGIN_PROPERTIES(object);

	switch (property_id) {
	case PROPERTY_PLUGIN:
		set_plugin(self, (GelPlugin *) g_value_get_pointer(value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_plugin_properties_dispose (GObject *object)
{
	G_OBJECT_CLASS (eina_plugin_properties_parent_class)->dispose (object);
}

static void
eina_plugin_properties_class_init (EinaPluginPropertiesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPluginPropertiesPrivate));

	object_class->get_property = eina_plugin_properties_get_property;
	object_class->set_property = eina_plugin_properties_set_property;
	object_class->dispose = eina_plugin_properties_dispose;

	g_object_class_install_property(object_class, PROPERTY_PLUGIN,
		g_param_spec_pointer("plugin", "Gel Plugin", "GelPlugin pointer to display",
		G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT
		));
}

static void
eina_plugin_properties_init (EinaPluginProperties *self)
{
	
	gchar *objs[] = { "main-widget", NULL };
	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (gtk_builder_add_objects_from_string(builder, ui_xml, -1, objs, &error) == 0)
	{
		g_warning("%s", error->message);
		g_error_free(error);
		return;
	}

	gtk_container_add(
		(GtkContainer *) gtk_dialog_get_content_area((GtkDialog *) self),
		(GtkWidget    *)  gtk_builder_get_object(builder, "main-widget"));
	gtk_dialog_add_button((GtkDialog *) self, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	EinaPluginPropertiesPrivate *priv = GET_PRIVATE(self);
	priv->name       = GTK_LABEL(gtk_builder_get_object(builder, "plugin-name"));
	priv->icon       = GTK_IMAGE(gtk_builder_get_object(builder, "plugin-icon"));
	priv->short_desc = GTK_LABEL(gtk_builder_get_object(builder, "plugin-short-desc"));
	priv->long_desc  = GTK_LABEL(gtk_builder_get_object(builder, "plugin-long-desc"));

	priv->pathname = GTK_LABEL(gtk_builder_get_object(builder, "plugin-pathname"));

	priv->deps  = GTK_LABEL(gtk_builder_get_object(builder, "plugin-deps"));
	priv->rdeps = GTK_LABEL(gtk_builder_get_object(builder, "plugin-rdeps"));

	g_object_unref(builder);
}

EinaPluginProperties*
eina_plugin_properties_new (GelPlugin *plugin)
{
	return g_object_new (EINA_TYPE_PLUGIN_PROPERTIES, "plugin", plugin, NULL);
}

void
set_info(EinaPluginProperties *self, GelPluginInfo *info)
{
	g_return_if_fail(plugin != NULL);

	EinaPluginPropertiesPrivate *priv = GET_PRIVATE(self);
	priv->info = info;

	gchar *path = plugin->icon ? gel_plugin_get_resource(plugin, GEL_RESOURCE_UI, (gchar *) plugin->icon) : NULL;
	if (info->icon_pathname)
	{
		gtk_image_set_from_file(priv->icon, info->icon_pathname);
		g_free(path);
	}
	else
		gtk_image_set_from_stock(priv->icon, EINA_STOCK_PLUGIN, GTK_ICON_SIZE_DIALOG); 

	gtk_label_set_markup(priv->name, gel_str_or_text(info->name, N_("No name")));
	gtk_label_set_markup(priv->short_desc, gel_str_or_text(info->short_desc, N_("No description")));
	gtk_label_set_markup(priv->long_desc,  gel_str_or_text(info->long_desc,  N_("No more information")));

	gtk_label_set_markup(priv->pathname, gel_str_or_text(info->pathname, N_("[Internal plugin]")));

	gtk_label_set_markup(priv->deps,  gel_str_or_text(plugin->depends, N_("No dependencies")));

	gchar *tmp = gel_plugin_stringify_dependants(plugin);
	gtk_label_set_markup(priv->rdeps, gel_str_or_text(tmp, N_("No reverse dependencies")));
	gel_free_and_invalidate(tmp);
}

const GelPlugin*
eina_plugin_properties_get_plugin(EinaPluginProperties *self)
{
	return (const GelPlugin *) GET_PRIVATE(self)->plugin;
}
