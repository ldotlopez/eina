/*
 * eina/template.c
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

#define GEL_DOMAIN "Eina::Template"

#include <config.h>
#include <gmodule.h>
#include <eina/template.h>
#include <eina/eina-plugin.h>

// --
// Define ourselves 
// --
struct _EinaTemplate {
	EinaObj  parent;

	// Add your own fields here
	gpointer dummy;
};

// --
// init/fini hooks
// --
static gboolean 
template_init (GelApp *app, EinaPlugin *plugin, GError **error)
{
	EinaTemplate *self;

	// Initialize ourself and automagically register into app using EinaObj
	self = g_new0(EinaTemplate, 1);
	if (!eina_obj_init(EINA_OBJ(self), plugin, "template", EINA_OBJ_NONE, error))
	{	
		g_free(self);
		return FALSE;
	}

	// If you dont use EinaObj save yourself into data field of EinaPlugin
	// remember to define in the top of this file EINA_PLUGIN_DATA_TYPE to
	// EinaTemplate
	// plugin->data = self

	// You may want to get info from another plugin (some sort of dependence)
	/*
	EinaFoo *foo;
	if ((foo = eina_obj_require(self, "foo", error)) == NULL)
		return FALSE;
	*/

	return TRUE;
}

static gboolean
template_fini (GelApp *app, EinaPlugin *plugin, GError **error)
{
	// Find yourself, rememer your macros in template.h?
	EinaTemplate *self = GEL_APP_GET_TEMPLATE(app);

	// Or use data field:
	// self = EINA_PLUGIN_DATA(plugin)

	// Unref dependences
	/*
	eina_obj_unrequire(self, "foo", NULL);
	*/

	// Free ourserves
	eina_obj_fini(EINA_OBJ(self));
	return TRUE;
}

// --
// Plugin definition
// --
G_MODULE_EXPORT EinaPlugin template_plugin = {
	EINA_PLUGIN_SERIAL,
	"template", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in template plugin"), NULL, NULL,

	template_init, template_fini,

	NULL, NULL, NULL
};

