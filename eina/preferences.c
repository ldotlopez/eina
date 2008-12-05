#define GEL_DOMAIN "Eina::Preferences"

#include <glib.h>
#include <gel/gel.h>
#include <eina/eina-preferences-dialog.h>
#include "base.h"
#include "player.h"
#include "preferences.h"


static void
menu_activate_cb(GtkAction *action, EinaPreferencesDialog *dialog)
{
	if (g_str_equal(gtk_action_get_name(action), "Preferences"))
	{
		gtk_dialog_run(GTK_DIALOG(dialog));
	}
}

static guint ui_merge_id = 0;
static GtkActionGroup *ag = NULL;
static GtkActionEntry action_entries[] = {
	{ "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences"),
	"<Control>p", NULL, G_CALLBACK(menu_activate_cb) },
};

G_MODULE_EXPORT gboolean preferences_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaPlayer            *player;
	GtkUIManager          *ui_manager;
	GError                *error = NULL;
	EinaPreferencesDialog *dialog = NULL;

	// Pre-requisites
	if (!(player = GEL_HUB_GET_PLAYER(hub)))
	{
		gel_error("Cannot get pre-requisite component 'player'");
		return FALSE;
	}
	if ((ui_manager = eina_player_get_ui_manager(player)) == NULL)
	{
		gel_error("Cannot access UI Manager");
		return FALSE;
	}

	dialog = eina_preferences_dialog_new();
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_hide), dialog);
	g_signal_connect(G_OBJECT(dialog), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), dialog);

	ui_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Preferences\" action=\"Preferences\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, &error);
	if (ui_merge_id == 0)
	{
		gel_warn("Cannot merge UI: '%s'", error->message);
		g_error_free(error);
		goto preferences_init_fail;
	}

	ag = gtk_action_group_new("preferences");
	gtk_action_group_add_actions(ag, action_entries, G_N_ELEMENTS(action_entries), dialog);
	gtk_ui_manager_insert_action_group(ui_manager, ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

	if (!gel_hub_shared_set(hub, "preferences", dialog))
	{
		gel_error("Cannot set shared mem");
		goto preferences_init_fail;
	}

	return TRUE;

preferences_init_fail:
	if (dialog)
		gtk_widget_destroy(GTK_WIDGET(dialog));
	if (ui_merge_id)
		gtk_ui_manager_remove_ui(ui_manager, ui_merge_id);
	if (ag)
		gtk_ui_manager_remove_action_group(ui_manager, ag);
	if (dialog)
		gtk_widget_destroy(GTK_WIDGET(dialog));
	return FALSE;
}

G_MODULE_EXPORT gboolean preferences_exit
(gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(data));
	return TRUE;
}

G_MODULE_EXPORT GelHubSlave preferences_connector = {
	"settings",
	&preferences_init,
	&preferences_exit
};

