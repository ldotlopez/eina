#define GEL_DOMAIN "Eina::Base"
#include <gel/gel-ui.h>
#include "base.h"

gboolean eina_base_init
(EinaBase *self, GHub *hub, gchar *name, EinaBaseFlag flags)
{
	GError *err = NULL;

	/* Try to get our place in hub */
	if (!g_hub_shared_set(hub, name, (gpointer) self)) {
		return FALSE;
	}

	self->name = g_strdup(name);
	self->hub  = hub;
	self->lomo = (LomoPlayer *) g_hub_shared_get(hub, "lomo");

	if (flags & EINA_BASE_GTK_UI) {
		// self->ui = gtk_ext_load_ui(self->name);
		self->ui = gel_ui_load_resource(self->name, &err);
		if (!self->ui) {
			gel_error("Error creating GelUI for '%s': %s", name, err->message);
			g_error_free(err);
			g_free(self->name);
			return FALSE;
		}
	}

	return TRUE;
}

void eina_base_fini(EinaBase *self) {
	if (self->ui != NULL) {
		g_object_unref(self->ui);
	}

	g_free(self->name);
	g_free(self);
}

GHub *eina_base_get_hub(EinaBase *self) {
	return self->hub;
}

LomoPlayer *eina_base_get_lomo(EinaBase *self) {
	return self->lomo;
}

GtkBuilder *eina_base_get_ui(EinaBase *self) {
	return self->ui;
}

