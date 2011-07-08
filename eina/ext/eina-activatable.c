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
#include <gel/gel.h>
#include <libpeas/peas.h>

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
GEL_DEFINE_QUARK_FUNC(eina_activatable)

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
	static gboolean initialized = FALSE;

	if (!initialized)
	{
		/**
		 * EinaActivatable:application:
		 *
		 * The application property contains the targetted application for this
		 * #EinaActivatable instance.
		 * It is set at construction time and won't change.
		 */
		g_object_interface_install_property (iface,
			g_param_spec_object ("application",
				"Application",
				"Application",
				EINA_TYPE_APPLICATION,
				G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
		initialized = TRUE;
	}
}

/**
 * eina_activatable_activate:
 * @application: An #EinaApplication
 * @activatable: An #EinaActivatable
 * @error: Location for returned error, or %NULL
 *
 * Activates an #EinaActivatable
 *
 * Returns: %TRUE if successfull, %FALSE otherwise.
 */
gboolean
eina_activatable_activate(EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	if (!EINA_IS_ACTIVATABLE(activatable) || !EINA_IS_APPLICATION(application))
	{
		g_set_error(error, eina_activatable_quark(), EINA_ACTIVATABLE_INVALID_ARGS, "Invalid arguments");
		g_return_val_if_fail (EINA_IS_APPLICATION (application), FALSE);
		g_return_val_if_fail (EINA_IS_ACTIVATABLE (activatable), FALSE);
	}

	gboolean ret = FALSE;
	EinaActivatableInterface *iface = EINA_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->activate != NULL)
		ret = iface->activate(activatable, application, error);

	if (!ret && error && !(*error)) 
		g_set_error(error, eina_activatable_quark(), EINA_ACTIVATABLE_UNKNOW_ERROR, "Unknow error");

	return ret;
}

/**
 * eina_activatable_deactivate:
 * @activatable: An #EinaActivatable
 * @application: An #EinaApplication
 * @error: Location for returned error, or %NULL
 *
 * Deactivates an #EinaActivatable
 *
 * Returns: %TRUE if successfull, %FALSE otherwise.
 */
gboolean
eina_activatable_deactivate(EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	if (!EINA_IS_ACTIVATABLE(activatable) || !EINA_IS_APPLICATION(application))
	{
		g_set_error(error, eina_activatable_quark(), EINA_ACTIVATABLE_INVALID_ARGS, "Invalid arguments");
		g_return_val_if_fail (EINA_IS_APPLICATION (application), FALSE);
		g_return_val_if_fail (EINA_IS_ACTIVATABLE (activatable), FALSE);
	}

	gboolean ret = FALSE;
	EinaActivatableInterface *iface = EINA_ACTIVATABLE_GET_IFACE (activatable);

	if (iface->deactivate != NULL)
		ret = iface->deactivate (activatable, application, error);

	if (!ret && error && !(*error))
		g_set_error(error, eina_activatable_quark(), EINA_ACTIVATABLE_UNKNOW_ERROR, "Unknow error");

	return ret;
}

/**
 * eina_activatable_get_application:
 * @activatable: An #EinaApplication
 *
 * Get application from @activatable
 *
 * Returns: (transfer none): An #EinaApplication
 */
EinaApplication*
eina_activatable_get_application(EinaActivatable *activatable)
{
	g_return_val_if_fail(EINA_IS_ACTIVATABLE(activatable), NULL);

	EinaApplication *application = NULL;
	g_object_get(G_OBJECT(activatable), "application", &application, NULL);
	return EINA_APPLICATION(application);
}

