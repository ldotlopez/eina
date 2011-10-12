/*
 * eina/dock/eina-dock.h
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

#ifndef __EINA_DOCK_H__
#define __EINA_DOCK_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <eina/dock/eina-dock-tab.h>

G_BEGIN_DECLS

#define EINA_TYPE_DOCK eina_dock_get_type()
#define EINA_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_DOCK, EinaDock))
#define EINA_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_DOCK, EinaDockClass))
#define EINA_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_DOCK))
#define EINA_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_DOCK))
#define EINA_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_DOCK, EinaDockClass))

typedef struct _EinaDockPrivate EinaDockPrivate;
typedef struct {
	GtkBox parent;
	EinaDockPrivate *priv;
} EinaDock;

typedef struct {
	GtkBoxClass parent_class;
	void (*widget_add)    (EinaDock *self, const gchar *id);
	void (*widget_remove) (EinaDock *self, const gchar *id);
} EinaDockClass;

/**
 * EinaDockFlag:
 * @EINA_DOCK_FLAG_DEFAULT: Add #EinaDockTab whatever fits better
 * @EINA_DOCK_FLAG_PRIMARY: Add #EinaDockTab as primary if possible
 *
 * Define how #EinaDockTab are added to #EinaDock
 */
typedef enum {
	EINA_DOCK_FLAG_DEFAULT = 0,
	EINA_DOCK_FLAG_PRIMARY = 1
} EinaDockFlag;

GType eina_dock_get_type (void);

EinaDock  *eina_dock_new (void);

gchar**  eina_dock_get_page_order(EinaDock *self);
void     eina_dock_set_page_order(EinaDock *self, gchar **order);

void     eina_dock_set_resizable(EinaDock *self, gboolean resizable);
gboolean eina_dock_get_resizable(EinaDock *self);

void     eina_dock_set_expanded(EinaDock *self, gboolean expanded);
gboolean eina_dock_get_expanded(EinaDock *self);

EinaDockTab *eina_dock_add_widget (EinaDock *self,
	const gchar *id, GtkWidget *widget, GtkWidget *label, EinaDockFlag flags);
gboolean eina_dock_remove_widget  (EinaDock *self, EinaDockTab *tab);
gboolean eina_dock_switch_widget  (EinaDock *self, EinaDockTab *tab);
guint    eina_dock_get_n_widgets  (EinaDock *self);

G_END_DECLS

#endif // __EINA_DOCK_H__
