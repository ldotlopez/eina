/*
 * eina/ext/eina-preferences-tab.c
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

#include "eina-preferences-tab.h"
#include "eina-ext-marshallers.h"
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>

G_DEFINE_TYPE (EinaPreferencesTab, eina_preferences_tab, GTK_TYPE_VBOX)

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

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};
static guint preferences_tab_signals[LAST_SIGNAL] = { 0 };

struct _EinaPreferencesTabPrivate {
	GtkImage  *label_image;
	gchar     *label_text;
	GtkWidget *label_widget;
	GList     *watching;
};

static void
preferences_tab_redo_widget(EinaPreferencesTab *self);
static void
toggle_toggled_cb (GtkToggleButton *w, EinaPreferencesTab *self);
static void
combo_box_changed_cb(GtkComboBox *w, EinaPreferencesTab *self);
static gboolean
entry_focus_out_event_cb(GtkEntry *w, GdkEventFocus *ev, EinaPreferencesTab *self);

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

	if (priv->watching)
	{
		g_list_foreach(priv->watching, (GFunc) g_free, NULL);
		g_list_free(priv->watching);
		priv->watching = NULL;
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

	preferences_tab_signals[SIGNAL_CHANGED] = g_signal_new ("changed",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (EinaPreferencesTabClass, changed),
		NULL, NULL,
		eina_ext_marshal_VOID__STRING_POINTER,
		G_TYPE_NONE,
		2,
		G_TYPE_STRING,
		G_TYPE_VALUE);
}

static void
eina_preferences_tab_init (EinaPreferencesTab *self)
{
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);
	if (0) priv = NULL;
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
		gtk_box_pack_start((GtkBox *) self, widget, FALSE, FALSE, 0);
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
eina_preferences_tab_add_watcher (EinaPreferencesTab *self, gchar  *object )
{
	gchar *objs[2] = { object,  NULL };
	eina_preferences_tab_add_watchers(self, objs);
}

void
eina_preferences_tab_add_watchers(EinaPreferencesTab *self, gchar **objects)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	if (!objects)
		return;
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	gint i;
	for (i = 0; objects && objects[i]; i++)
	{
		GtkWidget *widget = gel_ui_container_find_widget((GtkContainer *) self, objects[i]);
		if (!widget)
		{
			g_warning(N_("Object '%s' not found in %p"), objects[i], self);
			continue;
		}

		if (GTK_IS_TOGGLE_BUTTON(widget))
			g_signal_connect (widget, "toggled", (GCallback) toggle_toggled_cb, self);
		else if (GTK_IS_COMBO_BOX(widget))
			g_signal_connect (widget, "changed", (GCallback) combo_box_changed_cb, self);
		else if (GTK_IS_ENTRY(widget))
		{
			gtk_widget_add_events(widget, GDK_FOCUS_CHANGE_MASK);
			g_signal_connect (widget, "focus-out-event", (GCallback) entry_focus_out_event_cb, self);
		}
		else
		{
			g_warning(N_("Type '%s' is not handled"), G_OBJECT_TYPE_NAME(widget));
			continue;
		}
		if (!g_list_find(priv->watching, objects[i]))
			priv->watching = g_list_prepend(priv->watching, g_strdup(objects[i]));
	}
}

void
eina_preferences_tab_remove_watcher (EinaPreferencesTab *self, gchar  *object )
{
	gchar *objs[2] = { object,  NULL };
	eina_preferences_tab_remove_watchers(self, objs);
}

void
eina_preferences_tab_remove_watchers(EinaPreferencesTab *self, gchar **objects)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));

	if (!objects)
		return;
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);

	gint i;
	for (i = 0; objects && objects[i]; i++)
	{
		GtkWidget *widget = gel_ui_container_find_widget((GtkContainer *) self, objects[i]);
		if (!widget)
		{
			g_warning(N_("Object '%s' not found in %p"), objects[i], self);
			continue;
		}

		if (GTK_IS_TOGGLE_BUTTON(widget))
			g_signal_handlers_disconnect_by_func(widget, toggle_toggled_cb, self);
		else if (GTK_IS_COMBO_BOX(widget))
			g_signal_handlers_disconnect_by_func(widget, combo_box_changed_cb, self);
		else if (GTK_IS_ENTRY(widget))
			g_signal_handlers_disconnect_by_func(widget, entry_focus_out_event_cb, self);
		else
		{
			g_warning(N_("Type '%s' is not handled"), G_OBJECT_TYPE_NAME(widget));
			continue;
		}

		GList *llink = g_list_find(priv->watching, objects[i]);
		if (llink)
		{
			g_free(llink->data);
			priv->watching = g_list_remove_link(priv->watching, llink);
			g_list_free(llink);
		}
	}

}

GList *
eina_preferences_tab_get_watched(EinaPreferencesTab *self)
{
	return g_list_copy(GET_PRIVATE(self)->watching);
}


gboolean
eina_preferences_tab_set_widget_value(EinaPreferencesTab *self, gchar *name, GValue *value)
{
	g_return_val_if_fail(EINA_IS_PREFERENCES_TAB(self), FALSE);

	GtkWidget *widget = gel_ui_container_find_widget((GtkContainer *) self, name);
	g_return_val_if_fail(widget != NULL, FALSE);

	if (GTK_IS_TOGGLE_BUTTON(widget))
	{
		g_return_val_if_fail(G_VALUE_HOLDS_BOOLEAN(value), FALSE);
		gtk_toggle_button_set_active((GtkToggleButton *) widget, g_value_get_boolean(value));
	}
	else if (GTK_IS_COMBO_BOX(widget))
	{
		g_return_val_if_fail(G_VALUE_HOLDS_INT(value), FALSE);
		gtk_combo_box_set_active((GtkComboBox *) widget, g_value_get_int(value));
	}
	else if (GTK_IS_ENTRY(widget))
	{
		g_return_val_if_fail(G_VALUE_HOLDS_STRING(value), FALSE);
		gtk_entry_set_text((GtkEntry *) widget, g_value_get_string(value));
	}
	else
	{
		g_warning(N_("Type '%s' is not handled"), G_OBJECT_TYPE_NAME(widget));
		return FALSE;
	}
	return TRUE;
}

gboolean
eina_preferences_tab_get_widget_value(EinaPreferencesTab *self, gchar *name, GValue *value)
{
	g_return_val_if_fail(EINA_IS_PREFERENCES_TAB(self), FALSE);

	GtkWidget *widget = gel_ui_container_find_widget((GtkContainer *) self, name);
	g_return_val_if_fail(widget != NULL, FALSE);

	if (GTK_IS_TOGGLE_BUTTON(widget))
	{
		g_value_init(value, G_TYPE_BOOLEAN);
		g_value_set_boolean(value, gtk_toggle_button_get_active((GtkToggleButton *) widget));
	}
	else if (GTK_IS_COMBO_BOX(widget))
	{
		g_value_init(value, G_TYPE_INT);
		g_value_set_int(value, gtk_combo_box_get_active((GtkComboBox *) widget));
	}
	else if (GTK_IS_ENTRY(widget))
	{
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, gtk_entry_get_text((GtkEntry *) widget));
	}
	else
	{
		g_warning(N_("Type '%s' is not handled"), G_OBJECT_TYPE_NAME(widget));
		return FALSE;
	}
	return TRUE;
}

static void
preferences_tab_redo_widget(EinaPreferencesTab *self)
{
	g_return_if_fail(EINA_IS_PREFERENCES_TAB(self));
	EinaPreferencesTabPrivate *priv = GET_PRIVATE(self);


	GtkImage  *image = priv->label_image;
	gchar     *text  = priv->label_text;

	if (!image)
		priv->label_image = image = g_object_ref_sink((GtkImage *) gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR));
	if (!text)
		priv->label_text = text = g_strdup("(null)");

	GtkLabel *label = (GtkLabel *) gtk_label_new(text);
	gtk_widget_show((GtkWidget *) image);
	gtk_widget_show((GtkWidget *) label);

	gel_free_and_invalidate(priv->label_widget, NULL, gtk_widget_destroy);

	priv->label_widget = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start((GtkBox *) priv->label_widget, (GtkWidget *) image, FALSE, TRUE, 0);
	gtk_box_pack_start((GtkBox *) priv->label_widget, (GtkWidget *) label, FALSE, TRUE, 0);
	g_object_ref_sink(priv->label_widget);
}

static void
toggle_toggled_cb (GtkToggleButton *w, EinaPreferencesTab *self)
{
	GValue *gv = g_new0(GValue, 1);

	g_value_init(gv, G_TYPE_BOOLEAN);
	g_value_set_boolean(gv, gtk_toggle_button_get_active(w));
	g_signal_emit(self, preferences_tab_signals[SIGNAL_CHANGED], 0, gtk_widget_get_name((GtkWidget *) w), gv);
	g_free(gv);
}

static void
combo_box_changed_cb(GtkComboBox *w, EinaPreferencesTab *self)
{
	GValue *gv = g_new0(GValue, 1);

	g_value_init(gv, G_TYPE_INT);
	g_value_set_int(gv, gtk_combo_box_get_active(w));
	g_signal_emit(self, preferences_tab_signals[SIGNAL_CHANGED], 0, gtk_widget_get_name((GtkWidget *) w), gv);
	g_free(gv);
}

static gboolean
entry_focus_out_event_cb(GtkEntry *w, GdkEventFocus *ev, EinaPreferencesTab *self)
{
	GValue *gv = g_new0(GValue, 1);

	g_value_init(gv, G_TYPE_STRING);
	g_value_set_static_string(gv, gtk_entry_get_text((GtkEntry *) w));
	g_signal_emit(self, preferences_tab_signals[SIGNAL_CHANGED], 0, gtk_widget_get_name((GtkWidget *) w), gv);
	g_free(gv);

	return FALSE;
}

