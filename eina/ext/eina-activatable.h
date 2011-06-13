/*
 * eina-activatable.h
 *
 * Copyright (C) 2010 - Steve Frécinaux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EINA_ACTIVATABLE_H__
#define __EINA_ACTIVATABLE_H__

#include <glib-object.h>
#include <eina/ext/eina-application.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define EINA_TYPE_ACTIVATABLE           (eina_activatable_get_type ())
#define EINA_ACTIVATABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj),    EINA_TYPE_ACTIVATABLE, EinaActivatable))
#define EINA_ACTIVATABLE_IFACE(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),       EINA_TYPE_ACTIVATABLE, EinaActivatableInterface))
#define EINA_IS_ACTIVATABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj),    EINA_TYPE_ACTIVATABLE))
#define EINA_ACTIVATABLE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), EINA_TYPE_ACTIVATABLE, EinaActivatableInterface))

typedef struct _EinaActivatable          EinaActivatable; /* dummy typedef */
typedef struct _EinaActivatableInterface EinaActivatableInterface;

enum {
	EINA_ACTIVATABLE_UNKNOW_ERROR = 1,
	EINA_ACTIVATABLE_INVALID_ARGS 
};

typedef struct _EinaActivatableInterfacePrivate EinaActivatableInterfacePrivate;
struct _EinaActivatableInterface {
	GTypeInterface g_iface;
	EinaActivatableInterfacePrivate *priv;

	/* Virtual public methods */
	#ifdef EINA_ACTIVATABLE_SIMPLE_API
	void (*activate)   (EinaActivatable *activatable, EinaApplication *application);
	void (*deactivate) (EinaActivatable *activatable, EinaApplication *application);
	#else
	gboolean (*activate)   (EinaActivatable *activatable, EinaApplication *application, GError **error);
	gboolean (*deactivate) (EinaActivatable *activatable, EinaApplication *application, GError **error);
	#endif
};

/*
 * Public methods
 */
GType eina_activatable_get_type   (void)  G_GNUC_CONST;

#ifdef EINA_ACTIVATABLE_SIMPLE_API
void  eina_activatable_activate   (EinaActivatable *activatable, EinaApplication *application);
void  eina_activatable_deactivate (EinaActivatable *activatable, EinaApplication *application);
#else
gboolean eina_activatable_activate  (EinaActivatable *activatable, EinaApplication *application, GError **error);
gboolean eina_activatable_deactivate(EinaActivatable *activatable, EinaApplication *application, GError **error);

void     eina_activatable_set_data  (EinaActivatable *activatable, gpointer data);
gpointer eina_activatable_get_data  (EinaActivatable *activatable);
gpointer eina_activatable_steal_data(EinaActivatable *activatable);

EinaApplication *eina_activatable_get_application(EinaActivatable *activatable);
const gchar     *eina_activatable_get_data_dir(EinaActivatable *activatable);


#endif
G_END_DECLS

#endif /* __EINA_ACTIVATABLE_H__ */
