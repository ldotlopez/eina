#ifndef __EINA_LOMO_PLUGIN_H__
#define __EINA_LOMO_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

G_BEGIN_DECLS

#define EINA_TYPE_LOMO_PLUGIN         (eina_lomo_plugin_get_type ())
#define EINA_LOMO_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_LOMO_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EINA_TYPE_LOMO_PLUGIN, EinaLomoPlugin))
#define EINA_IS_LOMO_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_LOMO_PLUGIN))
#define EINA_IS_LOMO_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EINA_TYPE_LOMO_PLUGIN))
#define EINA_LOMO_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EINA_TYPE_LOMO_PLUGIN, EinaLomoPluginClass))

typedef struct _EinaLomoPlugin       EinaLomoPlugin;
typedef struct _EinaLomoPluginClass  EinaLomoPluginClass;

struct _EinaLomoPlugin {
  PeasExtensionBase parent_instance;
};

struct _EinaLomoPluginClass {
  PeasExtensionBaseClass parent_class;
};

GType                 eina_lomo_plugin_get_type        (void) G_GNUC_CONST;
G_MODULE_EXPORT void  peas_register_types                         (PeasObjectModule *module);

G_END_DECLS

#endif /* __EINA_LOMO_PLUGIN_H__ */
