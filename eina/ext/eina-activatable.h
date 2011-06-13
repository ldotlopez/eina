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

struct _EinaActivatableInterface {
  GTypeInterface g_iface;

  /* Virtual public methods */
  void (*activate)   (EinaActivatable *activatable, EinaApplication *application);
  void (*deactivate) (EinaActivatable *activatable, EinaApplication *application);
};

/*
 * Public methods
 */

EinaActivatableInterface* eina_activatable_get_iface(GObject *object);

GType eina_activatable_get_type   (void)  G_GNUC_CONST;
void  eina_activatable_activate   (EinaActivatable *activatable, EinaApplication *application);
void  eina_activatable_deactivate (EinaActivatable *activatable, EinaApplication *application);

G_END_DECLS

#endif /* __EINA_ACTIVATABLE_H__ */
