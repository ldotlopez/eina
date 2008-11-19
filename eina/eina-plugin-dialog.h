#ifndef _EINA_PLUGIN_DIALOG
#define _EINA_PLUGIN_DIALOG

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_PLUGIN_DIALOG eina_plugin_dialog_get_type()

#define EINA_PLUGIN_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialog))

#define EINA_PLUGIN_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogClass))

#define EINA_IS_PLUGIN_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PLUGIN_DIALOG))

#define EINA_IS_PLUGIN_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PLUGIN_DIALOG))

#define EINA_PLUGIN_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PLUGIN_DIALOG, EinaPluginDialogClass))

typedef struct {
  GtkDialog parent;
} EinaPluginDialog;

typedef struct {
  GtkDialogClass parent_class;
} EinaPluginDialogClass;

GType eina_plugin_dialog_get_type (void);

EinaPluginDialog* eina_plugin_dialog_new (void);

G_END_DECLS

#endif /* _EINA_PLUGIN_DIALOG */
