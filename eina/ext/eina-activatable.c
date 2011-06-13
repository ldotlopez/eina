/*
 * eina-activatable.c
 *
 * Copyright (C) 2010 Steve Frécinaux
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eina-activatable.h"

/**
 * SECTION:eina-activatable
 * @short_description: Interface for activatable plugins.
 * @see_also: #PeasExtensionSet
 *
 * #EinaActivatable is an interface which should be implemented by plugins
 * that should be activated on an object of a certain type (depending on the
 * application). For instance, in a typical windowed application,
 * #EinaActivatable plugin instances could be bound to individual toplevel
 * windows.
 *
 * It is typical to use #EinaActivatable along with #PeasExtensionSet in order
 * to activate and deactivate extensions automatically when plugins are loaded
 * or unloaded.
 *
 * You can also use the code of this interface as a base for your own
 * extension types, as illustrated by gedit's #GeditWindowActivatable and
 * #GeditDocumentActivatable interfaces.
 **/

G_DEFINE_INTERFACE(EinaActivatable, eina_activatable, G_TYPE_OBJECT)

/**
 * eina_activatable_get_iface:
 * @object: A #GObject
 *
 * Gets the #EinaActivatableInterface from @object
 *
 * Returns: (transfer none): The #EinaActivatableInterface
 */
EinaActivatableInterface*
eina_activatable_get_iface(GObject *object)
{
	return EINA_ACTIVATABLE_GET_IFACE(object);
}

void
eina_activatable_default_init (EinaActivatableInterface *iface)
{
}

/**
 * eina_activatable_activate:
 * @activatable: A #EinaActivatable.
 *
 * Activates the extension on the targetted object.
 *
 * On activation, the extension should hook itself to the object
 * where it makes sense.
 */
void
eina_activatable_activate (EinaActivatable *activatable, EinaApplication *application)
{
	EinaActivatableInterface *iface;

	g_return_if_fail (EINA_IS_ACTIVATABLE (activatable));
	g_return_if_fail (EINA_IS_APPLICATION (application));

	iface = EINA_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->activate != NULL)
		iface->activate (activatable, application);
}

/**
 * eina_activatable_deactivate:
 * @activatable: A #EinaActivatable.
 *
 * Deactivates the extension on the targetted object.
 *
 * On deactivation, an extension should remove itself from all the hooks it
 * used and should perform any cleanup required, so it can be unreffed safely
 * and without any more effect on the host application.
 */
void
eina_activatable_deactivate (EinaActivatable *activatable, EinaApplication *application)
{
	EinaActivatableInterface *iface;

	g_return_if_fail (EINA_IS_ACTIVATABLE (activatable));
	g_return_if_fail (EINA_IS_APPLICATION (application));

	iface = EINA_ACTIVATABLE_GET_IFACE (activatable);
	if (iface->deactivate != NULL)
		iface->deactivate (activatable, application);
}

