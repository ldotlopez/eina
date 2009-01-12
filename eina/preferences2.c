#define GEL_DOMAIN "Eina::Preferences"

#include <config.h>
#include <eina/player2.h>
#include <eina/preferences2.h>

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

enum {
	EINA_PREFERENCES_NO_ERROR = 0,
	EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
	EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
	EINA_PREFERENCES_CANNOT_REGISTER_SHARED
};

static GQuark
preferences_quark(void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string("preferences");
	return ret;
}

static gboolean
preferences_init (GelPlugin *plugin, GError **error)
{
	GelApp                *app = gel_plugin_get_app(plugin);
	EinaPlayer            *player;
	GtkUIManager          *ui_manager;
	EinaPreferencesDialog *dialog = NULL;

	// Pre-requisites
	if (!(player = GEL_APP_GET_PLAYER(app)))
	{
		g_set_error(error, preferences_quark(), EINA_PREFERENCES_CANNOT_ACCESS_PLAYER,
			N_("Cannot get pre-requisite component 'player'"));
		return FALSE;
	}
	if ((ui_manager = eina_player_get_ui_manager(player)) == NULL)
	{
		g_set_error(error, preferences_quark(), EINA_PREFERENCES_CANNOT_ACCESS_UI_MANAGER,
			N_("Cannot get UI Manager"));
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
		-1, error);
	if (ui_merge_id == 0)
		goto preferences_init_fail;

	ag = gtk_action_group_new("preferences");
	gtk_action_group_add_actions(ag, action_entries, G_N_ELEMENTS(action_entries), dialog);
	gtk_ui_manager_insert_action_group(ui_manager, ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

	if (!gel_app_shared_set(app, "preferences", dialog))
	{
		g_set_error(error, preferences_quark(), EINA_PREFERENCES_CANNOT_REGISTER_SHARED,
			N_("Cannot get register preferences"));
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

static gboolean preferences_fini
(GelPlugin *plugin, GError **error)
{
	gtk_widget_destroy(GTK_WIDGET(GEL_APP_GET_PREFERENCES(gel_plugin_get_app(plugin))));
	return TRUE;
}

G_MODULE_EXPORT GelPlugin preferences_plugin = {
	GEL_PLUGIN_SERIAL,
	"settings", PACKAGE_VERSION,
	N_("Build-in settings plugin"), NULL,
	NULL, NULL, NULL,
	preferences_init, preferences_fini,
	NULL, NULL
};

