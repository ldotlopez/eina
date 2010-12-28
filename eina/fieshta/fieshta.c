#include <eina/eina-plugin.h>
#include <eina/lomo/lomo.h>
#include "eina-fiestha-behaviour.h"

typedef struct {
	gpointer magic;
	guint ui_mng_merge_id;
	gboolean enabled;
	EinaFieshtaBehaviour *behaviour;
} Fieshta;
#define FIESHTA(x) ((Fieshta *) x)

static void
action_toggled_cb(GtkToggleAction *action, GelPlugin *plugin);

static gchar* ui_mng_str =
"<ui>"
"  <menubar name='Main' >"
"    <menu name='Tools' action='plugins-menu' >"
"      <menuitem name='Party mode' action='party-action' />"
"    </menu>"
"  </menubar>"
"</ui>";

static GtkToggleActionEntry ui_mng_actions[] =
{
	{ "party-action", NULL, N_("Party mode"), NULL, NULL, (GCallback) action_toggled_cb , FALSE }
};

G_MODULE_EXPORT gboolean
fieshta_plugin_init(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	Fieshta *self = g_new0(Fieshta, 1);

	eina_plugin_window_action_group_add_toggle_actions(plugin, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions));
	self->ui_mng_merge_id = eina_plugin_window_ui_manager_add_from_string(plugin, ui_mng_str);

	gel_plugin_set_data(plugin, self);
	return TRUE;
}

G_MODULE_EXPORT gboolean
fieshta_plugin_fini(EinaApplication *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

static void
fieshta_enable(EinaPlugin *plugin)
{
	Fieshta *self = FIESHTA(gel_plugin_get_data(plugin));
	if (self->enabled)
		return;

	self->behaviour = eina_fieshta_behaviour_new(eina_plugin_get_lomo(plugin), EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT);
	self->enabled = TRUE;
}

static void
fieshta_disable(EinaPlugin *plugin)
{
	Fieshta *self = FIESHTA(gel_plugin_get_data(plugin));
	if (!self->enabled)
		return;

	g_object_unref(self->behaviour);
	self->enabled = FALSE;
}


static void
action_toggled_cb(GtkToggleAction *action, GelPlugin *plugin)
{
	const gchar *name = gtk_action_get_name((GtkAction *) action);
	gboolean state = gtk_toggle_action_get_active(action);

	if (g_str_equal(name, "party-action"))
	{
		g_warning("Party mode: %s", state ? "on" : "off");
		if (state)
			fieshta_enable(plugin);
		else
			fieshta_disable(plugin);
	}
}
