#define GEL_DOMAIN "Eina::Base"

#include <gel/gel-ui.h>
#include <eina/base.h>

gboolean eina_base_init
(EinaBase *self, GelHub *hub, gchar *name, EinaBaseFlag flags)
{
	GError *err = NULL;

	if (!gel_hub_shared_set(hub, name, (gpointer) self))
		return FALSE;

	self->name = g_strdup(name);
	self->hub  = hub;
	self->lomo = (LomoPlayer *) gel_hub_shared_get(hub, "lomo");

	if (flags & EINA_BASE_GTK_UI)
	{
		self->ui = gel_ui_load_resource(self->name, &err);
		if (self->ui == NULL)
		{
			gel_error(_("Error creating GelUI for '%s': %s"), name, err->message);
			g_error_free(err);
			eina_base_fini(self);
			return FALSE;
		}
	}

	return TRUE;
}

void eina_base_fini(EinaBase *self)
{
	if (self == NULL)
	{
		gel_warn(_("Trying to free a NULL pointer"));
		return;
	}
		
	if (self->ui != NULL)
		g_object_unref(self->ui);
	if (self->name != NULL)
		g_free(self->name);

	g_free(self);
}

gpointer
eina_base_require(EinaBase *self, gchar *component)
{
	gpointer ret = NULL;
	if (!gel_hub_load(self->hub,component))
		return NULL;
	if ((ret = gel_hub_shared_get(self->hub, component)) == NULL)
		gel_hub_unload(self->hub,component);
	return ret;
}

GelHub *eina_base_get_hub(EinaBase *self)
{
	return self->hub;
}

LomoPlayer *eina_base_get_lomo(EinaBase *self)
{
	return self->lomo;
}

GtkBuilder *eina_base_get_ui(EinaBase *self)
{
	return self->ui;
}
