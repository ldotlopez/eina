#define GEL_DOMAIN "Gel"
#include "gel-ui.h"
/*
 * UI creation
 */
GelUI *
gel_ui_load_resource(gchar *ui_filename, GError **error)
{
	GelUI *ret = NULL;
	gchar *ui_pathname;
	gchar *tmp;

	tmp = g_strconcat(ui_filename, ".gtkui", NULL);
	// XXX: Handle GError
	ui_pathname = gel_app_resource_get_pathname(GEL_APP_RESOURCE_UI, tmp);
	g_free(tmp);

	if (ui_pathname == NULL) {
		return NULL;
	}

	ret = gtk_builder_new();
	if (gtk_builder_add_from_file(ret, ui_pathname, error) == 0) {
		gel_error("Error loading GtkBuilder: %s", (*error)->message);
		g_object_unref(ret);
		ret = NULL;
	}

	return ret;
}

/*
 * Signal handling
 */
gboolean
gel_ui_signal_connect_from_def(GelUI *ui, GelUISignalDef def, gpointer data, GError **error)
{
	GtkWidget *widget;

	widget = (GtkWidget*) gtk_builder_get_object(ui, def.widget);

	if ((widget == NULL) || !GTK_WIDGET(widget))
	{ 
		gel_warn("Can not find widget '%s' at GelUI %p", def.widget, ui);
		return FALSE;
	}

	g_signal_connect(widget, def.signal, def.callback, data);
	return TRUE;
}

gboolean
gel_ui_signal_connect_from_def_multiple(GelUI *ui, GelUISignalDef defs[], gpointer data, guint *count)
{
	guint _count = 0;
	gboolean ret = TRUE;
	guint i;
	
	for (i = 0; defs[i].widget != NULL; i++)
	{
		if (gel_ui_signal_connect_from_def(ui, defs[i], data, NULL))
			_count++;
		else
			ret = FALSE;
	}

	if (count != NULL)
		*count = _count;
	return ret;
}

/*
 * Images on UI's
 */
GdkPixbuf *
gel_ui_load_pixbuf_from_imagedef(GelUIImageDef def, GError **error)
{
	GdkPixbuf *ret;

	gchar *pathname = gel_app_resource_get_pathname(GEL_APP_RESOURCE_IMAGE, def.image);
	if (pathname == NULL)
	{
		// Handle error
		return NULL;
	}

	ret = gdk_pixbuf_new_from_file_at_size(pathname, def.w, def.h, error);
	g_free(pathname);
	return ret;
}

gboolean
gel_ui_load_image_from_def(GelUI *ui, GelUIImageDef *def, GError **error);

gboolean
gel_ui_load_image_from_def_multiple(GelUI *ui, GelUIImageDef defs[], guint *count);

G_END_DECLS
