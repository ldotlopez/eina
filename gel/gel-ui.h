#ifndef _GEL_UI_H
#define _GEL_UI_H

#include <gtk/gtk.h>
#include <gel/gel.h>

G_BEGIN_DECLS

#define GelUI GtkBuilder

typedef struct GelUISignalDef
{
	gchar     *widget;
	gchar     *signal;
	GCallback  callback;
} GelUISignalDef;
#define GEL_UI_SIGNAL_DEF_NONE { NULL, NULL, NULL }

typedef struct GelUIImageDef
{
	gchar *widget;
	gchar *image;
	gint w, h;
}
GelUIImageDef;
#define GEL_UI_IMAGE_DEF_NONE { NULL, NULL, -1, -1 }

/*
 * UI creation
 */
GelUI *
gel_ui_load_resource(gchar *ui_filename, GError **error);

/*
 * Signal handling
 */
gboolean
gel_ui_signal_connect_from_def(GelUI *ui, GelUISignalDef def, gpointer data, GError **error);

gboolean
gel_ui_signal_connect_from_def_multiple(GelUI *ui, GelUISignalDef defs[], gpointer data, guint *count);

/*
 * Images on UI's
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error);

gboolean
gel_ui_load_image_from_def(GelUI *ui, GelUIImageDef *def, GError **error);

gboolean
gel_ui_load_image_from_def_multiple(GelUI *ui, GelUIImageDef defs[], guint *count);

G_END_DECLS

#endif // _GEL_UI_H
