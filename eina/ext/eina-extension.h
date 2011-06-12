#ifndef __EINA_EXTENSION_H__
#define __EINA_EXTENSION_H__

#include <eina/ext/eina-application.h>
#include <eina/ext/eina-activatable.h>

#define EINA_DEFINE_EXTENSION(TypeName, type_name, _G_TYPE_NAME) \
 \
static void type_name##_activate   (EinaActivatable *activatable, EinaApplication *application); \
static void type_name##_deactivate (EinaActivatable *activatable, EinaApplication *application); \
static void type_name##_class_init (TypeName##Class *klass)     { } \
static void type_name##_class_finalize (TypeName##Class *klass) { } \
static void type_name##_init           (TypeName *plugin)       { } \
 \
static void eina_activatable_iface_init (EinaActivatableInterface *iface) { \
	iface->activate   = type_name##_activate;   \
	iface->deactivate = type_name##_deactivate; \
} \
 \
G_DEFINE_DYNAMIC_TYPE_EXTENDED (TypeName, \
	type_name, \
	PEAS_TYPE_EXTENSION_BASE, \
	0, \
	G_IMPLEMENT_INTERFACE_DYNAMIC (EINA_TYPE_ACTIVATABLE, \
	eina_activatable_iface_init)) \
 \
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module) { \
	type_name##_register_type (G_TYPE_MODULE (module)); \
 \
	peas_object_module_register_extension_type (module, \
		EINA_TYPE_ACTIVATABLE, \
		_G_TYPE_NAME); \
} \

#endif
