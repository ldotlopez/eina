#ifndef __EINA_EXTENSION_H__
#define __EINA_EXTENSION_H__

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gel/gel.h>
#include <libpeas/peas.h>
#include <eina/ext/eina-activatable.h>

#define EINA_DEFINE_EXTENSION_HEADERS(TypeName,type_name) \
	typedef struct _##TypeName        TypeName;        \
	typedef struct _##TypeName##Class TypeName##Class; \
	struct _##TypeName        { PeasExtensionBase      parent_instance; }; \
	struct _##TypeName##Class { PeasExtensionBaseClass parent_class;    }; \
	GType                 type_name##_get_type (void) G_GNUC_CONST;        \
	G_MODULE_EXPORT void  peas_register_types  (PeasObjectModule *module);

#define EINA_DEFINE_EXTENSION(TypeName,type_name,_G_TYPE_NAME) \
	\
	static void eina_activatable_iface_init (EinaActivatableInterface *iface); \
	static void type_name##_init            (TypeName *plugin)       { } \
	static void type_name##_class_init      (TypeName##Class *klass) { } \
	static void type_name##_class_finalize  (TypeName##Class *klass) { } \
	\
	G_DEFINE_DYNAMIC_TYPE_EXTENDED (TypeName, \
		type_name,                            \
		PEAS_TYPE_EXTENSION_BASE,             \
		0,                                    \
		G_IMPLEMENT_INTERFACE_DYNAMIC (EINA_TYPE_ACTIVATABLE, eina_activatable_iface_init)) \
	\
	static gboolean type_name##_activate   (EinaActivatable *activatable, EinaApplication *application, GError **error); \
	static gboolean type_name##_deactivate (EinaActivatable *activatable, EinaApplication *application, GError **error); \
	\
	static void eina_activatable_iface_init (EinaActivatableInterface *iface) { \
		iface->activate   = type_name##_activate;   \
		iface->deactivate = type_name##_deactivate; \
	}                                               \
	\
	G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module) { \
		type_name##_register_type (G_TYPE_MODULE (module));                 \
		peas_object_module_register_extension_type (module,                 \
		EINA_TYPE_ACTIVATABLE,  \
		_G_TYPE_NAME);          \
	}

#endif

