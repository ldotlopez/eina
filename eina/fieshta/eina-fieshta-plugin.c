/*
 * eina/fieshta/eina-fieshta-plugin.c
 *
 * Copyright (C) 2004-2011 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eina-fieshta-behaviour.h"
#include <eina/ext/eina-extension.h>
#include <eina/lomo/eina-lomo-plugin.h>

/**
 * EinaExtension boilerplate code
 */
#define EINA_TYPE_FIESHTA_PLUGIN         (eina_fieshta_plugin_get_type ())
#define EINA_FIESHTA_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPlugin))
#define EINA_FIESHTA_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPlugin))
#define EINA_IS_FIESHTA_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EINA_TYPE_FIESHTA_PLUGIN))
#define EINA_IS_FIESHTA_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    EINA_TYPE_FIESHTA_PLUGIN))
#define EINA_FIESHTA_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPluginClass))

typedef struct {
	guint ui_mng_merge_id;
	gboolean enabled;
	EinaFieshtaBehaviour *behaviour;
} EinaFieshtaPluginPrivate;
EINA_PLUGIN_REGISTER(EINA_TYPE_FIESHTA_PLUGIN, EinaFieshtaPlugin, eina_fieshta_plugin)

static void fieshta_enable (EinaFieshtaPlugin *plugin);
static void fieshta_disable(EinaFieshtaPlugin *plugin);
static void action_toggled_cb(GtkToggleAction *action, EinaFieshtaPlugin *plugin);

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

static gboolean
eina_fieshta_plugin_activate(EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	EinaFieshtaPlugin        *plugin = EINA_FIESHTA_PLUGIN(activatable);
	EinaFieshtaPluginPrivate *priv   = plugin->priv;

	priv->ui_mng_merge_id = 0;
	priv->behaviour = NULL;
	priv->enabled   = FALSE;

	GtkActionGroup *ag   = eina_application_get_window_action_group(application);
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(application);

	gtk_action_group_add_toggle_actions(ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), plugin);
	priv->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_mng_str, -1,  NULL);

	return TRUE;
}

static gboolean
eina_fieshta_plugin_deactivate(EinaActivatable *activatable, EinaApplication *application, GError **error)
{
	EinaFieshtaPlugin        *plugin = EINA_FIESHTA_PLUGIN(activatable);
	EinaFieshtaPluginPrivate *priv   = plugin->priv;

	fieshta_disable(plugin);

	GtkActionGroup *ag   = eina_application_get_window_action_group(application);
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(application);

	for (guint i = 0; i < G_N_ELEMENTS(ui_mng_actions); i++)
			gtk_action_group_remove_action(ag, GTK_ACTION(gtk_action_group_get_action(ag, ui_mng_actions[i].name)));
	if (priv->ui_mng_merge_id)
	{
		gtk_ui_manager_remove_ui(ui_mng, priv->ui_mng_merge_id);
		priv->ui_mng_merge_id = 0;
	}

	return TRUE;
}

static void
fieshta_enable(EinaFieshtaPlugin *plugin)
{
	g_return_if_fail(EINA_IS_FIESHTA_PLUGIN(plugin));
	EinaFieshtaPluginPrivate *priv = plugin->priv;

	if (priv->enabled)
		return;

	priv->behaviour = eina_fieshta_behaviour_new(
		eina_application_get_lomo(eina_activatable_get_application(EINA_ACTIVATABLE(plugin))),
		EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT);

	priv->enabled = (priv->behaviour != NULL);
}

static void
fieshta_disable(EinaFieshtaPlugin *plugin)
{
	g_return_if_fail(EINA_IS_FIESHTA_PLUGIN(plugin));
	EinaFieshtaPluginPrivate *priv = plugin->priv;

	if (!priv->enabled)
		return;

	g_warn_if_fail(EINA_IS_FIESHTA_BEHAVIOUR(priv->behaviour));
	if (priv->behaviour)
	{
		g_object_unref(priv->behaviour);
		priv->behaviour = NULL;
	}
	priv->enabled = (priv->behaviour != NULL);
}

static void
action_toggled_cb(GtkToggleAction *action, EinaFieshtaPlugin *plugin)
{
	const gchar *name = gtk_action_get_name((GtkAction *) action);
	gboolean state = gtk_toggle_action_get_active(action);

	if (g_str_equal(name, "party-action"))
	{
		g_debug("Party mode: %s", state ? "on" : "off");
		if (state)
			fieshta_enable(plugin);
		else
			fieshta_disable(plugin);
	}
}

