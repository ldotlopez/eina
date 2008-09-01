/* eina-io-dialog.h */

#ifndef _EINA_IO_DIALOG
#define _EINA_IO_DIALOG

#include <glib-object.h>

G_BEGIN_DECLS

#define EINA_TYPE_IO_DIALOG eina_io_dialog_get_type()

#define EINA_IO_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EINA_TYPE_IO_DIALOG, EinaIODialog))

#define EINA_IO_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EINA_TYPE_IO_DIALOG, EinaIODialogClass))

#define EINA_IS_IO_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EINA_TYPE_IO_DIALOG))

#define EINA_IS_IO_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EINA_TYPE_IO_DIALOG))

#define EINA_IO_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EINA_TYPE_IO_DIALOG, EinaIODialogClass))

typedef struct {
  GtkDialog parent;
} EinaIODialog;

typedef struct {
  GtkDialogClass parent_class;
} EinaIODialogClass;

GType eina_io_dialog_get_type (void);

EinaIODialog* eina_io_dialog_new (void);

G_END_DECLS

#endif /* _EINA_IO_DIALOG */

/* eina-io-dialog.c */

#include "eina-io-dialog.h"

G_DEFINE_TYPE (EinaIODialog, eina_io_dialog, GTK_DIALOG)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EINA_TYPE_IO_DIALOG, EinaIODialogPrivate))

typedef struct _EinaIODialogPrivate EinaIODialogPrivate;

struct _EinaIODialogPrivate {
};

static void
eina_io_dialog_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (eina_io_dialog_parent_class)->dispose)
    G_OBJECT_CLASS (eina_io_dialog_parent_class)->dispose (object);
}

static void
eina_io_dialog_class_init (EinaIODialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (EinaIODialogPrivate));

  object_class->dispose = eina_io_dialog_dispose;
}

static void
eina_io_dialog_init (EinaIODialog *self)
{
}

EinaIODialog*
eina_io_dialog_new (void)
{
  return g_object_new (EINA_TYPE_IO_DIALOG, NULL);
}

