/*
 * gel/gel-ui-generic.c
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


#include "gel-ui-generic.h"
#include <glib/gi18n.h>
#include "gel/gel.h"

G_DEFINE_TYPE (GelUIGeneric, gel_ui_generic, GTK_TYPE_BOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_UI_TYPE_GENERIC, GelUIGenericPrivate))

typedef struct _GelUIGenericPrivate GelUIGenericPrivate;

enum {
	PROP_XML_STRING = 1
};

struct _GelUIGenericPrivate {
	GtkBuilder *builder;
};

static void
set_xml_string(GelUIGeneric *self, gchar *xml_string);

static GObject*
gel_ui_generic_constructor(GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object = G_OBJECT_CLASS (gel_ui_generic_parent_class)->constructor (type, n_construct_params, construct_params);
	g_return_val_if_fail(object != NULL, NULL);

	for (guint i = 0; i < n_construct_params; i++)
	{
		GParamSpec *pspec = construct_params[i].pspec;
		GValue     *value = construct_params[i].value;
	
		if (pspec->owner_type != GEL_UI_TYPE_GENERIC)
			continue;

		if (g_str_equal(pspec->name, "xml-string"))
			set_xml_string((GelUIGeneric *) object, (gchar *) g_value_get_string(value));
	}
	return object;
}

static void
gel_ui_generic_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_ui_generic_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	if (pspec->flags & G_PARAM_CONSTRUCT_ONLY)
		return;

	switch (property_id) {
	case PROP_XML_STRING:
		return;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_ui_generic_dispose (GObject *object)
{
	GelUIGenericPrivate *priv = GET_PRIVATE((GelUIGeneric *) object);
	gel_free_and_invalidate(priv->builder, NULL, g_object_unref);

	G_OBJECT_CLASS (gel_ui_generic_parent_class)->dispose (object);
}

static void
gel_ui_generic_class_init (GelUIGenericClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelUIGenericPrivate));

	object_class->get_property = gel_ui_generic_get_property;
	object_class->set_property = gel_ui_generic_set_property;
	object_class->constructor = gel_ui_generic_constructor;
	object_class->dispose = gel_ui_generic_dispose;

	g_object_class_install_property(object_class, PROP_XML_STRING, 
		g_param_spec_string("xml-string", "xml-string", "xml-string",
		NULL, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gel_ui_generic_init (GelUIGeneric *self)
{
}

GtkWidget*
gel_ui_generic_new (gchar *xml_string)
{
	g_assert(xml_string != NULL);
	return g_object_new (GEL_UI_TYPE_GENERIC, "xml-string", xml_string, NULL);
}

GtkBuilder*
gel_ui_generic_get_builder(GelUIGeneric *self)
{
	g_return_val_if_fail(GEL_UI_IS_GENERIC(self), NULL);
	return GET_PRIVATE(self)->builder;
}

static void
set_xml_string(GelUIGeneric *self, gchar *xml_string)
{
	g_return_if_fail(GEL_UI_IS_GENERIC(self));
	g_return_if_fail(xml_string != NULL);

	GelUIGenericPrivate *priv = GET_PRIVATE(self);
	g_return_if_fail(priv->builder == NULL);

	priv->builder = gtk_builder_new();
	
	GError *error = NULL;
	if (!gtk_builder_add_from_string(priv->builder, xml_string, -1, &error))
	{
		g_warning(N_("Cannot load xml UI definition: %s"), error->message);
		g_error_free(error);
		return;
	}

	GtkWidget *w = GTK_WIDGET(gtk_builder_get_object(priv->builder, "main-widget"));
	if (w == NULL)
	{
		g_warning(N_("Unable to find widget 'main-widget' on xml UI definition"));
		return;
	}

	GtkWidget *p = gtk_widget_get_parent(w);
	gtk_widget_reparent(w, (GtkWidget *) self);
	gtk_box_set_child_packing ((GtkBox *) self, w, TRUE, TRUE, 0, GTK_PACK_START);
	g_object_unref(p);
}

