/*
 * eina/dock/eina-dock-tab.h
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

#ifndef __EINA_DOCK_TAB_H__
#define __EINA_DOCK_TAB_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_DOCK_TAB eina_dock_tab_get_type()

#define EINA_DOCK_TAB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_DOCK_TAB, EinaDockTab))
#define EINA_DOCK_TAB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EINA_TYPE_DOCK_TAB, EinaDockTabClass))
#define EINA_IS_DOCK_TAB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_DOCK_TAB))
#define EINA_IS_DOCK_TAB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EINA_TYPE_DOCK_TAB))
#define EINA_DOCK_TAB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EINA_TYPE_DOCK_TAB, EinaDockTabClass))

typedef struct _EinaDockTabPrivate EinaDockTabPrivate;
typedef struct {
	GObject parent;
	EinaDockTabPrivate *priv;
} EinaDockTab;

typedef struct {
	GObjectClass parent_class;
} EinaDockTabClass;

GType eina_dock_tab_get_type (void);

EinaDockTab* eina_dock_tab_new (const gchar *id, GtkWidget *widget, GtkWidget *label, gboolean primary);

const gchar* eina_dock_tab_get_id     (EinaDockTab *self);
GtkWidget*   eina_dock_tab_get_widget (EinaDockTab *self);
GtkWidget*   eina_dock_tab_get_label  (EinaDockTab *self);
gboolean     eina_dock_tab_get_primary(EinaDockTab *self);

void         eina_dock_tab_set_primary(EinaDockTab *self, gboolean primary);

gboolean     eina_dock_tab_equal(EinaDockTab *a, EinaDockTab *b);

G_END_DECLS

#endif

