#include <gtk/gtk.h>
#include "eina/ext/eina-application.h"

gint main(gint argc, gchar *argv[])
{
	EinaApplication *app = eina_application_new(&argc, &argv);
	gtk_widget_show_all((GtkWidget *) gtk_application_create_window((GtkApplication *) app));
	gtk_application_run(GTK_APPLICATION(app));

	return 0;
}


