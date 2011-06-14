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

#include "eina-fieshta-plugin.h"
#include "eina-fieshta-behaviour.h"
#include <eina/lomo/eina-lomo-plugin.h>

typedef struct {
	gpointer magic;
	guint ui_mng_merge_id;
	gboolean enabled;
	EinaFieshtaBehaviour *behaviour;
} EinaFieshtaData;
#define EINA_FIESHTA_DATA(x) ((EinaFieshtaData *) x)

static void
action_toggled_cb(GtkToggleAction *action, EinaActivatable *activatable);

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

EINA_DEFINE_EXTENSION(EinaFieshtaPlugin, eina_fieshta_plugin, EINA_TYPE_FIESHTA_PLUGIN)

static gboolean
eina_fieshta_plugin_activate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaFieshtaData *self = g_new0(EinaFieshtaData, 1);

	GtkActionGroup *ag   = eina_application_get_window_action_group(app);
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);

	gtk_action_group_add_toggle_actions(ag, ui_mng_actions, G_N_ELEMENTS(ui_mng_actions), activatable);
	self->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_mng_str, -1,  NULL);

	eina_activatable_set_data(activatable, self);
	return TRUE;
}

static gboolean
eina_fieshta_plugin_deactivate(EinaActivatable *activatable, EinaApplication *app, GError **error)
{
	EinaFieshtaData *self = (EinaFieshtaData *) eina_activatable_get_data(activatable);

	GtkActionGroup *ag   = eina_application_get_window_action_group(app);
	GtkUIManager *ui_mng = eina_application_get_window_ui_manager(app);
	
	gtk_ui_manager_remove_ui(ui_mng, self->ui_mng_merge_id);
	for (guint i = 0; i < G_N_ELEMENTS(ui_mng_actions); i++)
		gtk_action_group_remove_action(ag, (GtkAction *) &(ui_mng_actions[i]));
	return TRUE;
}

static void
fieshta_enable(EinaActivatable *activatable)
{
	EinaFieshtaData *self = EINA_FIESHTA_DATA(eina_activatable_get_data(activatable));
	if (self->enabled)
		return;

	self->behaviour = eina_fieshta_behaviour_new(
		eina_application_get_lomo(eina_activatable_get_application(activatable)),
		EINA_FIESHTA_BEHAVIOUR_OPTION_DEFAULT);
	self->enabled = TRUE;
}

static void
fieshta_disable(EinaActivatable *activatable)
{
	EinaFieshtaData *self = EINA_FIESHTA_DATA(eina_activatable_get_data(activatable));
	if (!self->enabled)
		return;

	g_object_unref(self->behaviour);
	self->enabled = FALSE;
}


static void
action_toggled_cb(GtkToggleAction *action, EinaActivatable* activatable)
{
	const gchar *name = gtk_action_get_name((GtkAction *) action);
	gboolean state = gtk_toggle_action_get_active(action);

	if (g_str_equal(name, "party-action"))
	{
		g_warning("Party mode: %s", state ? "on" : "off");
		if (state)
			fieshta_enable(activatable);
		else
			fieshta_disable(activatable);
	}
}

