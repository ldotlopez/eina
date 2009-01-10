#define GEL_DOMAIN "Eina::Obj"
#if USE_GEL_APP
#include <glib/gi18n.h>
#include <gel/gel-ui.h>
#include <eina/eina-obj.h>

static GQuark
eina_obj_quark(void)
{
	GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("eina-obj");
	return ret;
}

enum {
	EINA_OBJ_NO_ERROR = 0,
	EINA_OBJ_PLUGIN_NOT_FOUND,
	EINA_OBJ_SHARED_NOT_FOUND,
	EINA_OBJ_CANNOT_LOAD_UI
};

gboolean eina_obj_init
(EinaObj *self, GelApp *app, gchar *name, EinaObjFlag flags, GError **error)
{
	if (!gel_app_shared_set(app, name, (gpointer) self))
		return FALSE;

	self->name = g_strdup(name);
	self->app  = app;
	self->lomo = GEL_APP_GET_LOMO(app);

	if (flags & EINA_OBJ_GTK_UI)
	{
		self->ui = gel_ui_load_resource(self->name, error);
		if (self->ui == NULL)
		{
			eina_obj_fini(self);
			return FALSE;
		}
	}

	return TRUE;
}

void eina_obj_fini(EinaObj *self)
{
	if (self == NULL)
	{
		gel_warn(N_("Trying to free a NULL pointer"));
		return;
	}
		
	gel_free_and_invalidate(self->ui,   NULL, g_object_unref);
	gel_free_and_invalidate(self->name, NULL, g_free);
	gel_free_and_invalidate(self,       NULL, g_free);
}

gpointer
eina_obj_require(EinaObj *self, gchar *plugin_name, GError **error)
{
	GelApp *app = eina_obj_get_app(self);
	GelPlugin *plugin;
	gpointer ret = NULL;

	if ((plugin = gel_app_load_plugin_by_name(app, plugin_name)) == NULL)
	{
		g_set_error(error, eina_obj_quark(),
			EINA_OBJ_PLUGIN_NOT_FOUND, N_("Plugin %s cannot be found"), plugin_name);
		return NULL;
	}
	if (!gel_plugin_is_enabled(plugin) && !gel_app_init_plugin(app, plugin, error))
	{
		 gel_app_unload_plugin(app, plugin, NULL);
		 return NULL;
	}

	if ((ret = gel_app_shared_get(eina_obj_get_app(self), plugin_name)) == NULL)
	{
		g_set_error(error, eina_obj_quark(),
			EINA_OBJ_SHARED_NOT_FOUND, N_("Shared mem for %s not found"), plugin_name);
		gel_app_unload_plugin(eina_obj_get_app(self), plugin, NULL);
	}

	return ret;
}
#endif