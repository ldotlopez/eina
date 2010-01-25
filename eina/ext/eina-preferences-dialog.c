/*
 * eina/ext/eina-preferences-dialog.c
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

#define GEL_DOMAIN "Eina::EinaPreferencesDialog"
#include <eina/ext/eina-preferences-dialog.h>
#include <eina/ext/eina-preferences-dialog-marshallers.h>
#include <glib/gi18n.h>
#include <gel/gel.h>
#include <gel/gel-ui.h>

#define EINA_HIG_BOX_SPACING 5

G_DEFINE_TYPE (EinaPreferencesDialog, eina_preferences_dialog, GTK_TYPE_DIALOG)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogPrivate))

typedef struct _EinaPreferencesDialogPrivate EinaPreferencesDialogPrivate;

struct _EinaPreferencesDialogPrivate {
	GtkNotebook *notebook;
};

enum {
	VALUE_CHANGED,
	LAST_SIGNAL
};

guint eina_preferences_dialog_signals[LAST_SIGNAL] = { 0 };

static void
toggle_toggled_cb(GtkToggleButton *w, EinaPreferencesDialog *self);
static void
combo_box_changed_cb(GtkComboBox *w, EinaPreferencesDialog *self);
static gboolean
entry_focus_out_event_cb(GtkEntry *w, GdkEventFocus *ev, EinaPreferencesDialog *self);

static void
toggle_set_value(GtkToggleButton *toggle, gboolean val);
static void
combo_box_set_value(GtkComboBox *combo, gint val);
static void
entry_set_value(GtkEntry *entry, gchar *value);

static void
eina_preferences_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->dispose (object);
}

static void
eina_preferences_dialog_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize)
		G_OBJECT_CLASS (eina_preferences_dialog_parent_class)->finalize (object);
}

static void
eina_preferences_dialog_class_init (EinaPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EinaPreferencesDialogPrivate));

	object_class->dispose = eina_preferences_dialog_dispose;
	object_class->finalize = eina_preferences_dialog_finalize;
	eina_preferences_dialog_signals[VALUE_CHANGED] =
		g_signal_new ("value-changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (EinaPreferencesDialogClass, value_changed),
			NULL, NULL,
			eina_preferences_dialog_marshal_VOID__STRING_STRING_POINTER,
			G_TYPE_NONE,
			3,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_POINTER);
}

static void
eina_preferences_dialog_init (EinaPreferencesDialog *self)
{
}

EinaPreferencesDialog*
eina_preferences_dialog_new (void)
{
	EinaPreferencesDialog *self = g_object_new (EINA_TYPE_PREFERENCES_DIALOG, NULL);
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);

	priv->notebook = (GtkNotebook *) gtk_notebook_new();
	gtk_notebook_set_show_tabs(priv->notebook, TRUE);
	gtk_widget_show_all(GTK_WIDGET(priv->notebook));

	gtk_dialog_add_button(GTK_DIALOG(self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	gtk_box_pack_start(
#if GTK_CHECK_VERSION(2,14,0)
		GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))),
#else
		GTK_BOX(GTK_DIALOG(self)->vbox),
#endif
	 	GTK_WIDGET(priv->notebook),
		TRUE, TRUE, 0);

	if (0)
	{
		combo_box_set_value(NULL, 0);
		toggle_set_value(NULL, FALSE);
	}
	return self;
}

void
eina_preferences_dialog_add_tab(EinaPreferencesDialog *self, const gchar *group, GtkImage *icon, GtkLabel *label, GtkWidget *tab)
{
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	if (!icon)
		icon = (GtkImage *) gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	GtkHBox *box = (GtkHBox *) gtk_hbox_new(FALSE, EINA_HIG_BOX_SPACING);

	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(icon),  FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), FALSE, FALSE, 0);

	gtk_widget_set_name(tab, group);

	gtk_widget_show_all(GTK_WIDGET(box));
	gtk_widget_show(GTK_WIDGET(tab));
	gtk_notebook_append_page(priv->notebook, GTK_WIDGET(tab), GTK_WIDGET(box));

	if (gtk_notebook_get_n_pages(priv->notebook) > 1)
		gtk_notebook_set_show_tabs(priv->notebook, TRUE);
}

void
eina_preferences_dialog_remove_tab(EinaPreferencesDialog *self, GtkWidget *widget)
{
	struct _EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	gint n = gtk_notebook_page_num(priv->notebook, widget);
	if (n == -1)
	{
		gel_warn("Widget is not on notebook");
		return;
	}

	gtk_notebook_remove_page(priv->notebook, n);

	if (gtk_notebook_get_n_pages(priv->notebook) <= 1)
		gtk_notebook_set_show_tabs(priv->notebook, FALSE);
}

void
eina_preferences_dialog_add_tab_from_entry(EinaPreferencesDialog *self, EinaPreferencesDialogEntry *entry)
{
	return eina_preferences_dialog_add_tab_full(self, entry->group, entry->xml, entry->root, entry->objects, entry->n, entry->icon, entry->label);
}

void
eina_preferences_dialog_add_tab_full(EinaPreferencesDialog *self, gchar *group, gchar *xml, gchar *root, gchar **objects, guint n,
    GtkImage *icon, GtkLabel *label)
{

	GError *err = NULL;

	// Get widget
	GtkBuilder *builder = gtk_builder_new();
	if (gtk_builder_add_from_string(builder, xml, -1, &err) == 0)
	{
		gel_warn(("Cannot parse XML: %s"), err->message);
		g_error_free(err);
		return;
	}

	guint i;
	for (i = 0; i < n; i++)
	{
		GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, objects[i]));
		if (!widget)
		{
			gel_warn(("Widget %s not found"), objects[i]);
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
			gel_warn(N_("Type '%s' is not handled"), G_OBJECT_TYPE_NAME(widget));
	}

	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, root));
	if (gtk_widget_get_parent(widget))
		gtk_widget_unparent(widget);
	eina_preferences_dialog_add_tab(self, group, icon, label, widget);
}

EinaPreferencesDialogEntry *
eina_preferences_dialog_entry_new(gchar *group, gchar *xml, gchar *root, gchar **objects, guint n,
	GtkImage *icon, GtkLabel *label)
{
	EinaPreferencesDialogEntry *entry = g_new0(EinaPreferencesDialogEntry, 1);
	g_get_current_time(&(entry->stamp));
	entry->group = g_strdup(group);
	entry->root  = g_strdup(root);
	entry->objects = g_new0(gchar *, n);
	while (entry->n < n)
	{
		entry->objects[entry->n] = g_strdup(objects[entry->n]);
		entry->n++;
	}
	entry->xml  = g_strdup(xml);
	entry->icon = icon;
	entry->label = label;
	if (icon)
		g_object_ref(entry->icon);
	if (label)
		g_object_ref(entry->label);

	return entry;
}

void
eina_preferences_dialog_entry_free(EinaPreferencesDialogEntry *entry)
{
	g_free(entry->group);
	g_free(entry->root);
	if (entry->objects > 0 )
	{
		g_free(entry->objects[entry->n - 1]);
		entry->objects[entry->n - 1] = NULL;
		g_strfreev(entry->objects);
	}
	g_free(entry->xml);
	if (entry->icon)
		g_object_unref(entry->icon);
	if (entry->label)
		g_object_unref(entry->label);
}

gint
eina_preferecens_dialog_entry_cmp(EinaPreferencesDialogEntry *a, EinaPreferencesDialogEntry *b)
{
	if (a->stamp.tv_sec < b->stamp.tv_sec)
		return -1;
	else if (b->stamp.tv_sec > a->stamp.tv_sec)
		return 1;
	else
		return (gint) (b->stamp.tv_usec - a->stamp.tv_usec);
}

GList *
get_groups(EinaPreferencesDialog *self)
{
	GList *ret = NULL;
	GtkNotebook *nb = GET_PRIVATE(self)->notebook;

	guint npages = gtk_notebook_get_n_pages(nb);
	guint i;

	for (i = 0; i < npages; i++)
		ret = g_list_prepend(ret, gtk_notebook_get_nth_page(nb, i));

	return g_list_reverse(ret);
}

GtkWidget *
get_group(EinaPreferencesDialog *self, gchar *group)
{
	GList *groups = get_groups(self);
	GList *iter = groups;
	GtkWidget *ret = NULL;
	while (iter && (ret == NULL))
	{
		if (g_str_equal(gtk_widget_get_name((GtkWidget *) iter->data), group))
			ret = (GtkWidget *) iter->data;
		iter = iter->next;
	}
	g_list_free(groups);
	return ret;
}

void
eina_preferences_dialog_set_value(EinaPreferencesDialog *self, gchar *group, gchar *object, GValue *value)
{
	GList *groups = NULL;
	if (group != NULL)
		groups = g_list_append(NULL, get_group(self, group));
	else
		groups = get_groups(self);
	
	GtkWidget *w = NULL;
	GList *iter = groups;
	while (iter && (w == NULL))
	{
		w = gel_ui_container_find_widget((GtkContainer *) iter->data, object);
		iter = iter->next;
	}

	if (w == NULL)
	{
		gel_warn(N_("Object '%s' for group %s not found"), object, group ? group : "(null)");
		return;
	}

	if (GTK_IS_COMBO_BOX(w))
		combo_box_set_value((GtkComboBox *) w, g_value_get_int(value));

	else if (GTK_IS_TOGGLE_BUTTON(w))
		toggle_set_value((GtkToggleButton *) w, g_value_get_boolean(value));

	else if (GTK_IS_ENTRY(w))
		entry_set_value((GtkEntry *) w, (gchar *) g_value_get_string(value));

	else
		gel_warn(N_("Unable to handle type '%s' for object '%s' of group '%s'"), G_OBJECT_TYPE_NAME(w), group, object);
}

// Widget handlers
static void
emit_signal(EinaPreferencesDialog *self, GtkWidget *w, GValue *value)
{
	EinaPreferencesDialogPrivate *priv = GET_PRIVATE(self);
	const gchar *group = gtk_widget_get_name(gtk_notebook_get_nth_page(priv->notebook, gtk_notebook_get_current_page(priv->notebook)));
	const gchar *name  = gtk_widget_get_name((GtkWidget *) w);

	g_signal_emit(self, eina_preferences_dialog_signals[VALUE_CHANGED], 0, group, name, value);
	g_value_unset(value);
	g_free(value);
}

static void
toggle_toggled_cb(GtkToggleButton *w, EinaPreferencesDialog *self)
{
	GValue *v = g_new0(GValue, 1);
	g_value_init(v, G_TYPE_BOOLEAN);
	g_value_set_boolean(v, gtk_toggle_button_get_active(w));
	emit_signal(self, (GtkWidget *) w, v);
}

static void
combo_box_changed_cb(GtkComboBox *w, EinaPreferencesDialog *self)
{
	GtkTreeIter iter;
	if (!gtk_combo_box_get_active_iter(w, &iter))
	{
		gel_warn(N_("Cannot get active iter"));
		return;
	}

	gint value;
	GtkTreeModel *model = gtk_combo_box_get_model(w);
	gtk_tree_model_get(model, &iter, 0, &value, -1);

	GValue *v = g_new0(GValue, 1);
	g_value_init(v, G_TYPE_INT);
	g_value_set_int(v, value);
	emit_signal(self, (GtkWidget *) w, v);
}

static gboolean
entry_focus_out_event_cb(GtkEntry *w, GdkEventFocus *Ev, EinaPreferencesDialog *self)
{
	GValue *v = g_new0(GValue, 1);
	g_value_init(v, G_TYPE_STRING);
	g_value_set_string(v, gtk_entry_get_text(w));
	emit_signal(self, (GtkWidget *) w, v);

	return FALSE;
}

// Setters
static void
toggle_set_value(GtkToggleButton *toggle, gboolean val)
{
	gtk_toggle_button_set_active(toggle, val);
}

static void
combo_box_set_value(GtkComboBox *combo, gint val)
{
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;

	gint v;
	do {
		gtk_tree_model_get(model, &iter, 0, &v, -1);
		if (v == val)
		{
			gtk_combo_box_set_active_iter(combo, &iter);
			return;
		}
	} while (gtk_tree_model_iter_next(model, &iter));
}

static void
entry_set_value(GtkEntry *entry, gchar *value)
{
	gtk_entry_set_text(entry, value);
}
