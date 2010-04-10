/*
 * eina/ext/eina-preferences-tab.h
 *
 * Copyright (C) 2004-2009 Eina
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
/* eina-preferences-tab.h */

#ifndef _EINA_PREFERENCES_TAB
#define _EINA_PREFERENCES_TAB

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_PREFERENCES_TAB eina_preferences_tab_get_type()

#define EINA_PREFERENCES_TAB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PREFERENCES_TAB, EinaPreferencesTab))

#define EINA_PREFERENCES_TAB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PREFERENCES_TAB, EinaPreferencesTabClass))

#define EINA_IS_PREFERENCES_TAB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PREFERENCES_TAB))

#define EINA_IS_PREFERENCES_TAB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PREFERENCES_TAB))

#define EINA_PREFERENCES_TAB_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PREFERENCES_TAB, EinaPreferencesTabClass))

typedef struct {
  GtkVBox parent;
} EinaPreferencesTab;

typedef struct {
  GtkVBoxClass parent_class;
  void (*changed) (const gchar *key, GValue *value);
} EinaPreferencesTabClass;

GType eina_preferences_tab_get_type (void);

EinaPreferencesTab* eina_preferences_tab_new (void);

void
eina_preferences_tab_set_label_widget(EinaPreferencesTab *self, GtkWidget *label_widget);

void
eina_preferences_tab_set_label_image(EinaPreferencesTab *self, GtkImage *label_widget);

void
eina_preferences_tab_set_label_text(EinaPreferencesTab *self, gchar *text);

void
eina_preferences_tab_set_widget(EinaPreferencesTab *self, GtkWidget *widget);

void
eina_preferences_tab_set_ui_string(EinaPreferencesTab *self, gchar *ui_string);

GtkWidget*
eina_preferences_tab_get_widget(EinaPreferencesTab *self, gchar *name);

GtkWidget *
eina_preferences_tab_get_label_widget(EinaPreferencesTab *self);

void
eina_preferences_tab_add_watcher (EinaPreferencesTab *self, gchar  *object );
void
eina_preferences_tab_add_watchers(EinaPreferencesTab *self, gchar **objects);

void
eina_preferences_tab_remove_watcher (EinaPreferencesTab *self, gchar  *object );
void
eina_preferences_tab_remove_watchers(EinaPreferencesTab *self, gchar **objects);

GList *
eina_preferences_tab_get_watched(EinaPreferencesTab *self);

gboolean
eina_preferences_tab_set_widget_value(EinaPreferencesTab *self, gchar *name, GValue *value);
gboolean
eina_preferences_tab_get_widget_value(EinaPreferencesTab *self, gchar *name, GValue *value);

G_END_DECLS

#endif /* _EINA_PREFERENCES_TAB */
