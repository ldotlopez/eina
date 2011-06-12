/*
 * peasdemo-hello-world-plugin.c
 * This file is part of libpeas
 *
 * Copyright (C) 2009-2010 Steve Frécinaux
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libpeas/peas.h>
#include <eina/ext/eina-activatable.h>
#include "plugin.h"

static void eina_activatable_iface_init     (EinaActivatableInterface    *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EinaLomoPlugin,
	eina_lomo_plugin,
	PEAS_TYPE_EXTENSION_BASE,
	0,
	G_IMPLEMENT_INTERFACE_DYNAMIC (EINA_TYPE_ACTIVATABLE,
	eina_activatable_iface_init))
/*
enum {
  PROP_0,
  PROP_OBJECT
};

static void
eina_lomo_plugin_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  EinaLomoPlugin *plugin = PEASDEMO_HELLO_WORLD_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_OBJECT:
      plugin->window = GTK_WIDGET (g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
eina_lomo_plugin_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  EinaLomoPlugin *plugin = PEASDEMO_HELLO_WORLD_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_OBJECT:
      g_value_set_object (value, plugin->window);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
*/

static void
eina_lomo_plugin_init (EinaLomoPlugin *plugin)
{
  g_warning (G_STRFUNC);
}

static void
eina_lomo_plugin_finalize (GObject *object)
{
  // EinaLomoPlugin *plugin = EINA_LOMO_PLUGIN(object);

  g_warning (G_STRFUNC);

  G_OBJECT_CLASS (eina_lomo_plugin_parent_class)->finalize (object);
}

static void
eina_lomo_plugin_activate (EinaActivatable *activatable, EinaApplication *application)
{
	// EinaLomoPlugin *plugin = EINA_LOMO_PLUGIN(activatable);

	g_warning (G_STRFUNC);
}

static void
eina_lomo_plugin_deactivate (EinaActivatable *activatable, EinaApplication *application)
{
	// EinaLomoPlugin *plugin = EINA_LOMO_PLUGIN (activatable);
	g_warning (G_STRFUNC);
}

static void
eina_lomo_plugin_class_init (EinaLomoPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	// object_class->set_property = eina_lomo_plugin_set_property;
	// object_class->get_property = eina_lomo_plugin_get_property;
	object_class->finalize = eina_lomo_plugin_finalize;

	// g_object_class_override_property (object_class, PROP_OBJECT, "object");
}

static void
eina_activatable_iface_init (EinaActivatableInterface *iface)
{
	iface->activate   = eina_lomo_plugin_activate;
	iface->deactivate = eina_lomo_plugin_deactivate;
}

static void
eina_lomo_plugin_class_finalize (EinaLomoPluginClass *klass)
{
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eina_lomo_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
		EINA_TYPE_ACTIVATABLE,
		EINA_TYPE_LOMO_PLUGIN);
}
