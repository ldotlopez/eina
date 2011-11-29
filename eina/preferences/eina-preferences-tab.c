/*
 * eina/preferences/eina-preferences-tab.c
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

#include "eina-preferences-tab.h"
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>

G_DEFINE_TYPE (EinaPreferencesTab, eina_preferences_tab, GTK_TYPE_BOX)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PREFERENCES_TAB, EinaPreferencesTabPrivate))

typedef struct _EinaPreferencesTabPrivate EinaPreferencesTabPrivate;

enum {
	PROPERTY_LABEL_WIDGET = 1,
	PROPERTY_LABEL_IMAGE,
	PROPERTY_LABEL_TEXT,
	PROPERTY_WIDGET,
	PROPERTY_UI_STRING
};

typedef struct {
	gchar *domain;
	gchar *key;
	gchar *obj_name;
	gchar *property;
} EinaPreferencesTabBindDef;

struct _EinaPreferencesTabPrivate {
	GtkImage  *label_image;
	gchar     *label_text;
	GtkWidget *label_widget;
	GList     *binds;
	GList     *bind_defs;
};

static void
realize_cb(EinaPreferencesTab *self, gpointer data);
static void
preferences_tab_redo_widget(EinaPreferencesTab *self);

static void
eina_preferences_tab_get_property (GObject *object, guint property_id,
			                        GValue *value, GParamSpec *pspec)
{
	// EinaPreferencesTab *self = EINA_PREFERENCES_TAB(object);

	switch (property_id) {
	#if 0
	case PROPERTY_LABEL_WIDGET:
		g_value_set_object(value, eina_preferences_tab_get_label_widget(self));
		break;
	case PROPERTY_LABEL_IMAGE:
		g_value_set_object(value, eina_preferences_tab_get_label_image(self));
		break;
	case PROPERTY_LABEL_TEXT:
		g_value_set_string(value, eina_preferences_tab_get_label_text(self));
		break;
	case PROPERTY_WIDGET:
		g_value_set_object(value, eina_preferences_tab_get_widget(self));
		break;
	case PROPERTY_UI_STRING:
		g_value_set_string(value, eina_preferences_tab_get_ui_string(self));
		break;
	#endif
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_preferences_tab_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	EinaPreferencesTab *self = EINA_PREFERENCES_TAB(object);

	switch (property_id) {
	case PROPERTY_LABEL_WIDGET:
		eina_preferences_tab_set_label_widget(self, GTK_WIDGET(g_value_get_object(value)));
		break;
	case PROPERTY_LABEL_IMAGE:
		eina_preferences_tab_set_label_image(self, GTK_IMAGE(g_value_get_object(value)));
		break;
	case PROPERTY_LABEL_TEXT:
		eina_preferences_tab_set_label_text(self, (gchar *) g_value_get_string(value));
		break;
	case PROPERTY_WIDGET:
		eina_preferences_tab_set_widget(self, GTK_WIDGET(g_value_get_object(value)));
		break;
	case PROPERTY_UI_STRING:
		eina_preferences_tab_set_ui_string(self, (gchar *) g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eina_preferences_tab_dispose (GObject *object)
{
	EinaPreferencesTab *self = EINA_PREFERENCES_TAB(object);
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	if (priv->binds)
	{
		g_list_foreach(priv->binds, (GFunc) g_object_unref, NULL);
		g_list_free(priv->binds);
		priv->binds = NULL;
	}

	if (priv->bind_defs)
	{
		GList *iter = priv->bind_defs;
		while (iter)
		{
			EinaPreferencesTabBindDef *def = (EinaPreferencesTabBindDef *) iter->data;
			g_free(def->domain);
			g_free(def->key);
			g_free(def->obj_name);
			g_free(def->property);
			g_free(def);

			iter = iter->next;
		}
		g_list_free(priv->bind_defs);
		priv->bind_defs = NULL;
	}

	G_OBJECT_CLASS (eina_preferences_tab_parent_class)->dispose (object);
}

static void
eina_preferences_tab_class_init (EinaPreferencesTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPreferencesTabPrivate));
	object_class->get_property = eina_preferences_tab_get_property;
	object_class->set_property = eina_preferences_tab_set_property;
	object_class->dispose = eina_preferences_tab_dispose;

	g_object_class_install_property(object_class, PROPERTY_LABEL_WIDGET,
		g_param_spec_object("label-widget", "label-widget", "label-widget",
		GTK_TYPE_WIDGET, G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROPERTY_LABEL_IMAGE,
		g_param_spec_object("label-image", "label-image", "label-image",
		GTK_TYPE_IMAGE, G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROPERTY_LABEL_TEXT,
		g_param_spec_string("label-text", "label-text", "label-text",
		NULL, G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROPERTY_WIDGET,
		g_param_spec_object("widget", "widget", "widget",
		GTK_TYPE_WIDGET, G_PARAM_READABLE | G_PARAM_WRITABLE));

	g_object_class_install_property(object_class, PROPERTY_UI_STRING,
		g_param_spec_string("ui-string", "ui-string", "ui-string",
		NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
eina_preferences_tab_init (EinaPreferencesTab *self)
{
	g_object_set((GObject *) self, "margin", 12, NULL);
	gtk_orientable_set_orientation((GtkOrientable *) self, GTK_ORIENTATION_VERTICAL);
	g_signal_connect(self, "realize", (GCallback) realize_cb, NULL);
}

EinaPreferencesTab*
eina_preferences_tab_new (void)
{
	return g_object_new (EINA_TYPE_PREFERENCES_TAB, NULL);
}

void
eina_preferences_tab_set_label_widget(EinaPreferencesTab *self, GtkWidget *label_widget)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->label_widget, NULL, g_object_unref);
	gel_free_and_invalidate(priv->label_image,  NULL, g_object_unref);
	gel_free_and_invalidate(priv->label_text,   NULL, g_free);

	if (label_widget)
		priv->label_widget = g_object_ref(label_widget);
}

void
eina_preferences_tab_set_label_image(EinaPreferencesTab *self, GtkImage *label_image)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->label_image,  NULL, g_object_unref);
	priv->label_image = g_object_ref(label_image);

	preferences_tab_redo_widget(self);
}

void
eina_preferences_tab_set_label_text(EinaPreferencesTab *self, gchar *text)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	gel_free_and_invalidate(priv->label_text,  NULL, g_free);
	priv->label_text = g_strdup(text);

	preferences_tab_redo_widget(self);
}

void
eina_preferences_tab_set_widget(EinaPreferencesTab *self, GtkWidget *widget)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));

	GList *iter, *children = gtk_container_get_children((GtkContainer *) self);
	iter = children;
	while (iter)
	{
		gtk_container_remove((GtkContainer *) self, (GtkWidget *) iter->data);
		iter = iter->next;
	}
	g_list_free(children);

	if (widget)
	{
		gtk_widget_show(widget);
		gtk_box_pack_start((GtkBox *) self, widget, FALSE, FALSE, 0);
	}
}

void
eina_preferences_tab_set_ui_string(EinaPreferencesTab *self, gchar *ui_string)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));

	if (!ui_string)
	{
		eina_preferences_tab_set_widget(self, NULL);
		return;
	}

	GError *error = NULL;
	GtkBuilder *builder = gtk_builder_new();
	if (gtk_builder_add_from_string(builder, ui_string, -1, &error) == 0)
	{
		g_warning(N_("Cannot parse UI string: %s"), error->message);
		g_error_free(error);
		eina_preferences_tab_set_widget(self, NULL);
		g_object_unref(builder);
		return;
	}

	GtkWidget *main_widget = GTK_WIDGET(gtk_builder_get_object(builder, "main-widget"));
	if (!main_widget)
	{
		g_warning(N_("Object 'main-widget' not found in UI string"));
		eina_preferences_tab_set_widget(self, NULL);
		g_object_unref(builder);
		return;
	}

	gtk_widget_unparent(main_widget);
	eina_preferences_tab_set_widget(self, main_widget);
	g_object_unref(builder);
}

GtkWidget*
eina_preferences_tab_get_widget(EinaPreferencesTab *self, gchar *name)
{
	return gel_ui_container_find_widget(GTK_CONTAINER(self), name);
}

GtkWidget *
eina_preferences_tab_get_label_widget(EinaPreferencesTab *self)
{
	g_return_val_if_fail(EINA_IS_PREFERENCES_TAB(self), NULL);
	return GET_PRIVATE(self)->label_widget;
}

void
eina_preferences_tab_bind(EinaPreferencesTab *self, GSettings *settings, gchar *settings_key, gchar *object_name, gchar *property)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	g_return_if_fail(G_IS_SETTINGS(settings));
	g_return_if_fail(settings_key);
	g_return_if_fail(property);

	if (!object_name)
		object_name = settings_key;

	GObject *obj = G_OBJECT(gel_ui_container_find_widget(GTK_CONTAINER(self), object_name));
	g_return_if_fail(obj && G_IS_OBJECT(obj));

	g_settings_bind(settings, settings_key, obj, property, G_SETTINGS_BIND_DEFAULT);
}

void
eina_preferences_tab_bind_entries(EinaPreferencesTab *self, GSettings *settings, guint n_entries, EinaPreferencesTabEntry entries[])
{
	for (guint i = 0; i < n_entries; i++)
		eina_preferences_tab_bind(self, settings, entries[i].settings_key, entries[i].object_name, entries[i].property);
}

void
eina_preferences_tab_bindv(EinaPreferencesTab *self, ...)
{
	va_list args;
	va_start(args, self);

	GSettings *sets = NULL;
	while ((sets = va_arg(args, GSettings *)) != NULL)
	{
		if (!G_IS_SETTINGS(sets))
		{
			g_warning(N_("Invalid GSettings parameter"));
			break;
		}

		EinaPreferencesTabEntry entry[1];
		if (!(entry[0].settings_key = va_arg(args, gchar*)) ||
		    !(entry[0].object_name  = va_arg(args, gchar*)) ||
			!(entry[0].property     = va_arg(args, gchar*)))
		{
			g_warning(N_("Invalid parameters"));
			break;
		}
		eina_preferences_tab_bind_entries(self, sets, 1, entry);
	}
	va_end(args);
}

static void
realize_cb(EinaPreferencesTab *self, gpointer data)
{
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);
	GList *iter = priv->bind_defs;
	while (iter)
	{
		EinaPreferencesTabBindDef *def = (EinaPreferencesTabBindDef *) iter->data;
		GObject *obj = (GObject *) gel_ui_container_find_widget((GtkContainer *) self, def->obj_name);
		if (obj)
		{
			GSettings *s = g_settings_new(def->domain);
			g_settings_bind(s, def->key, obj, def->property, G_SETTINGS_BIND_DEFAULT);
			priv->binds = g_list_prepend(priv->binds, s);
		}
		else
			g_warning(N_("Missing widget named '%s'"), def->obj_name);

		iter = iter->next;
	}
}

static void
preferences_tab_redo_widget(EinaPreferencesTab *self)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);


	GtkImage  *image = priv->label_image;
	gchar     *text  = priv->label_text;

	#if 0
	if (!image)
		priv->label_image = image = g_object_ref_sink((GtkImage *) gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_SMALL_TOOLBAR));
	if (!text)
		priv->label_text = text = g_strdup("(null)");
	#endif

	gel_free_and_invalidate(priv->label_widget, NULL, gtk_widget_destroy);

	GtkLabel *label = (GtkLabel *) gtk_label_new(text);
	gtk_widget_show((GtkWidget *) label);

	priv->label_widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_set_homogeneous((GtkBox *) priv->label_widget, FALSE);

	if (priv->label_image)
	{
		gtk_widget_show((GtkWidget *) image);
		gtk_box_pack_start((GtkBox *) priv->label_widget, (GtkWidget *) image, FALSE, TRUE, 0);
	}
	gtk_box_pack_start((GtkBox *) priv->label_widget, (GtkWidget *) label, FALSE, TRUE, 0);
	g_object_ref_sink(priv->label_widget);
}

