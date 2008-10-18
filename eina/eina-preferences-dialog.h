#ifndef _EINA_PREFERENCES_DIALOG
#define _EINA_PREFERENCES_DIALOG

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EINA_TYPE_PREFERENCES_DIALOG eina_preferences_dialog_get_type()

#define EINA_PREFERENCES_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialog))

#define EINA_PREFERENCES_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogClass))

#define EINA_IS_PREFERENCES_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_PREFERENCES_DIALOG))

#define EINA_IS_PREFERENCES_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_PREFERENCES_DIALOG))

#define EINA_PREFERENCES_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_PREFERENCES_DIALOG, EinaPreferencesDialogClass))

typedef struct {
  GtkDialog parent;
} EinaPreferencesDialog;

typedef struct {
  GtkDialogClass parent_class;
} EinaPreferencesDialogClass;

GType eina_preferences_dialog_get_type (void);

void
eina_preferences_dialog_add_tab(EinaPreferencesDialog *self, GtkImage *icon, GtkLabel *label, GtkWidget *tab);

void
eina_preferences_dialog_remove_tab(EinaPreferencesDialog *self, GtkWidget *widget);

G_END_DECLS

#endif
