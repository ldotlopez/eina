/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Plugin engine for Eina, heavily based on the code from Rhythmbox,
 * which is based heavily on the code from eina.
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 *               2006 James Livingston  <jrl@ids.org.au>
 *               2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __EINA_PLUGIN_H__
#define __EINA_PLUGIN_H__

#include <glib/gi18n.h>
#include <libpeas/peas.h>
#include <gel/gel.h>
#include <eina/ext/eina-activatable.h>
#include <eina/ext/eina-application.h>

G_BEGIN_DECLS

#define EINA_PLUGIN_REGISTER_WITH_CODE(TYPE_NAME,TypeName,type_name,TYPE_CODE) \
typedef struct {                         \
	PeasExtensionBaseClass parent_class; \
} TypeName##Class;                   \
typedef struct {             \
	PeasExtensionBase parent;    \
	TypeName##Private *priv; \
} TypeName;            \
\
GType type_name##_get_type (void) G_GNUC_CONST; \
static gboolean type_name##_activate     (EinaActivatable *plugin, EinaApplication *application, GError **error); \
static gboolean type_name##_deactivate   (EinaActivatable *plugin, EinaApplication *application, GError **error); \
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);       \
static void eina_activatable_iface_init  (EinaActivatableInterface *iface); \
\
enum {               \
	PROP_0,	         \
	PROP_APPLICATION \
}; \
\
G_DEFINE_DYNAMIC_TYPE_EXTENDED (TypeName, \
	type_name,                                  \
	PEAS_TYPE_EXTENSION_BASE,                   \
	0,                                          \
	G_IMPLEMENT_INTERFACE_DYNAMIC (EINA_TYPE_ACTIVATABLE, eina_activatable_iface_init) TYPE_CODE) \
\
static void	eina_activatable_iface_init (EinaActivatableInterface *iface) { \
	iface->activate   = type_name##_activate;   \
	iface->deactivate = type_name##_deactivate; \
} \
\
static void set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) { \
	switch (prop_id) { \
	case PROP_APPLICATION:  \
		g_object_set_data_full (object, "application", g_value_dup_object (value), g_object_unref);	\
		break;        \
	default:          \
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); \
		break;         \
	} \
} \
\
static void	get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) { \
	switch (prop_id) { \
		case PROP_APPLICATION: \
		g_value_set_object (value, g_object_get_data (object, "object")); \
		break; \
	default: \
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); \
		break; \
	} \
} \
static void type_name##_class_init (TypeName##Class *klass) { \
	GObjectClass *object_class = G_OBJECT_CLASS (klass); \
	object_class->set_property = set_property; \
	object_class->get_property = get_property; \
	g_object_class_override_property (object_class, PROP_APPLICATION, "application"); \
	g_type_class_add_private (klass, sizeof (TypeName##Private)); \
} \
static void type_name##_class_finalize (TypeName##Class *klass) { } \
static void type_name##_init (TypeName *plugin) { \
	plugin->priv = G_TYPE_INSTANCE_GET_PRIVATE (plugin, TYPE_NAME, TypeName##Private); \
} \
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module) { \
	type_name##_register_type (G_TYPE_MODULE (module)); \
	peas_object_module_register_extension_type (module, EINA_TYPE_ACTIVATABLE, TYPE_NAME); \
}

#define EINA_PLUGIN_REGISTER(TYPE_NAME,TypeName,type_name) \
	EINA_PLUGIN_REGISTER_WITH_CODE(TYPE_NAME,TypeName,type_name,)

G_END_DECLS

#endif  /* __EINA_PLUGIN_H__ */

