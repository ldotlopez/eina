#define GEL_DOMAIN "Eina::Template"

#include "base.h"
#include "template.h"
#include <gmodule.h>
#include <gel/gel-ui.h>

/* * * * * * * * * * */
/* Define ourselves  */
/* * * * * * * * * * */
struct _EinaTemplate {
	EinaBase   parent;

	// Add your own fields here
	gpointer nothing;
};

/* * * * * * * * * */
/* Lomo Callbacks */
/* * * * * * * * * */
/* Add here callback prototipes for LomoPlayer events: */
/* void on_template_lomo_something(LomoPlayer *lomo, ..., EinaTemplate *self); */

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
/* Add here callback prototipes por UI widgets */
/* void on_template_name_type_event(GtkWidget *w, ..., EinaTemplate *self); */

/* * * * * * * * * * * */
/* Signal definitions  */
/* * * * * * * * * * * */
GelUISignalDef _template_signals[] = {
	/* { "nameType", "something", G_CALLBACK(on_template_lomo_something) }, */
	GEL_UI_SIGNAL_DEF_NONE
};

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean template_init
(GHub *hub, gint *argc, gchar ***argv)
{
	EinaTemplate *self;

	/*
	 * Create mem in hub (if needed)
	 */
	self = g_new0(EinaTemplate, 1);
	if (!eina_base_init((EinaBase *) self, hub, "template", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	/* At this point:
	 * - template.glade was automagicaly loaded if exists
	 * - You have 4 macros to access resources:
	 *   LOMO(self):   access to LomoPlayer
	 *   HUB(self):    access the GHub
	 *   UI(self):     access GladeXML from template.glade
	 *   W(self,name): returns the GtkWidget named 'name' from template.glade
	 */

	/*
	 * Build some no-glade UI widgets here
	 */
	
	/*
	 * Connect signals for previously created widgets
	 */
	/* glade_ext_multiple_signal_connect_from_def(UI(self), _template_signals, self); */
	/* g_signal_connect(self->some_custom_widget, "signal", G_CALLBACK(on_template_widget_type_signal), self); */

	/*
	 * Connect signals for LomoPlayer
	 */
	/* g_signal_connect(self->lomo, "something", G_CALLBACK(on_template_lomo_something), self); */

	/*
	 * Final setup and initial show
	 */
	/* gtk_widget_show(W(self->ui, "mainWindow")); */

	return TRUE;
}

G_MODULE_EXPORT gboolean template_exit
(gpointer data)
{
	/* 
	 * Free alocated memory (if any)
	 */
	EinaTemplate *self = (EinaTemplate *) data;

	eina_base_fini((EinaBase *) self);
	return TRUE;
}

/* * * * * * * * * * * * * */
/* Implement UI Callbacks  */
/* * * * * * * * * * * * * */

/* * * * * * * * * * ** * * */
/* Implement Lomo Callbacks */
/* * * * * * * * * * ** * * */

/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GHubSlave template_connector = {
	"template",
	&template_init,
	&template_exit
};

