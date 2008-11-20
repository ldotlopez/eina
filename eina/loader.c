#define GEL_DOMAIN "Eina::Loader"

#include <gmodule.h>
#include <eina/loader.h>
#include <eina/player.h>
#include <eina/eina-plugin-dialog.h>

struct _EinaLoader {
	EinaBase  parent;

	GList *plugins;
	GList *paths;

	EinaPlayer     *player;

	guint           ui_mng_merge_id;
	GtkActionGroup *ui_mng_ag;
};

static GList*
build_paths(void);

static void
menu_activate_cb(GtkAction *action, EinaLoader *self);

G_MODULE_EXPORT gboolean loader_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaLoader *self;

	self = g_new0(EinaLoader, 1);
	if (!eina_base_init(EINA_BASE(self), hub, "loader", EINA_BASE_NONE))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	// Build menu
	if (!gel_hub_load(hub, "player") || ((self->player = GEL_HUB_GET_PLAYER(hub)) == NULL))
	{
		gel_error("Player is not available so where are useless");
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}

	GtkUIManager *ui_manager = eina_player_get_ui_manager(self->player);
	GError *error = NULL;
	self->ui_mng_merge_id = gtk_ui_manager_add_ui_from_string(ui_manager,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Edit\" action=\"EditMenu\" >"
		"<menuitem name=\"Plugins\" action=\"Plugins\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, &error);
	if (self->ui_mng_merge_id == 0)
	{
		gel_warn("Cannot merge UI: '%s'", error->message);
		g_error_free(error);
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}

	GtkActionEntry action_entries[] = {
		{ "Plugins", NULL, N_("Plugins"),
		NULL, NULL, G_CALLBACK(menu_activate_cb) }
	};

	self->ui_mng_ag = gtk_action_group_new("plugins");
	gtk_action_group_add_actions(self->ui_mng_ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ui_mng_ag, 1);
	gtk_ui_manager_ensure_update(ui_manager);

	self->paths = build_paths();

	return TRUE;
}

G_MODULE_EXPORT gboolean loader_exit
(gpointer data)
{
	EinaLoader *self = EINA_LOADER(data);
	GList *iter;

	// Unmerge menu
	GtkUIManager *ui_mng;
	if ((ui_mng = eina_player_get_ui_manager(self->player)) != NULL)
	{
		gtk_ui_manager_remove_action_group(ui_mng, self->ui_mng_ag);
		gtk_ui_manager_remove_ui(ui_mng, self->ui_mng_merge_id);
		g_object_unref(self->ui_mng_ag);
	}

	// Unload all plugins. All plugins must be disabled while unloading
	iter = self->plugins;
	while (iter)
	{
		if (eina_plugin_is_enabled(EINA_PLUGIN(iter->data)))
			break;
		if (!eina_plugin_free(EINA_PLUGIN(iter->data)))
			break;
	}
	if (iter != NULL)
	{
		gel_error("Cannot exit: not all plugins are disabled (%s)", eina_plugin_get_pathname(EINA_PLUGIN(iter->data)));
		return FALSE;
	}
	g_list_free(self->plugins);

	// Free paths
	g_list_foreach(self->paths, (GFunc) g_free, NULL);
	g_list_free(self->paths);

	eina_base_fini(EINA_BASE(self));

	return TRUE;
}

static GList *
build_paths(void)
{
	GList *ret = NULL;
	gchar *tmp;
	gchar **tmp2;
	gint i;

	// Add system dir
	ret = g_list_prepend(ret, g_strdup(PACKAGE_LIB_DIR));

	// Add user dir
	tmp = (gchar *) g_getenv ("HOME");
	if (!tmp)
		tmp = (gchar *) g_get_home_dir();
	if (tmp)
		ret = g_list_prepend(ret, g_build_filename(tmp, ".eina", "plugins", NULL));

	// Add from $EINA_PLUGINS_PATH
	tmp = (gchar *) g_getenv("EINA_PLUGINS_PATH");
	if (tmp != NULL)
	{
		tmp2 = g_strsplit(tmp, ":", 0);
		for (i = 0; tmp2[i] != NULL; i++) {
			if (tmp2[i][0] == '\0')
				continue;
			ret = g_list_prepend(ret, g_strdup(tmp2[i]));
		}
		g_strfreev(tmp2);
	}

	return ret;
}

static void
menu_activate_cb(GtkAction *action, EinaLoader *self)
{
	EinaPluginDialog *w = eina_plugin_dialog_new();
	gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(GTK_WIDGET(w));
}

GList *
eina_loader_query_paths(EinaLoader *self)
{
	return self->paths;
}

static gboolean
is_already_loaded(EinaLoader *self, EinaPlugin *plugin)
{
	GList *iter = self->plugins;
	while (iter)
	{
		if (g_str_equal(EINA_PLUGIN(iter->data)->priv->pathname, plugin->priv->pathname))
			return TRUE;
		iter = iter->next;
	}
	return FALSE;
}

GList *
eina_loader_query_plugins(EinaLoader *self)
{
	GList *iter, *paths;
	GList *ret = NULL;

	iter = paths = eina_loader_query_paths(self);
	while (iter)
	{
		EinaPlugin *plugin;
		GList *e, *entries;
		e = entries = gel_dir_read(iter->data, TRUE, NULL);
		while (e)
		{
			gchar *name = g_path_get_basename(e->data);
			gchar *mod_name = g_module_build_path(e->data, name);
			gchar *symbol = g_strconcat(name, "_plugin", NULL); 
			gel_warn("Try: '%s'", mod_name);
			if ((plugin = eina_plugin_new(HUB(EINA_BASE(self)), mod_name, symbol)) != NULL)
			{
				gel_warn("Ok!");
				eina_plugin_free(plugin);
			}
			/*
			if ((plugin = eina_iface_query_plugin_by_path(self, mod_name)) != NULL)
				ret = g_list_prepend(ret, plugin);
			else if ((plugin = eina_iface_load_plugin_by_path(self, name, mod_name)) != NULL)
				ret = g_list_prepend(ret, plugin);
			*/
			g_free(symbol);
			g_free(mod_name);
			g_free(name);
			e = e->next;
		}
		gel_glist_free(entries, (GFunc) g_free, NULL);
		iter = iter->next;
	}
	gel_glist_free(paths, (GFunc) g_free, NULL);
	return g_list_reverse(ret);
}

EinaPlugin*
eina_loader_load_plugin(EinaLoader *self, gchar *pathname, GError **error)
{
	return NULL;
}

gboolean
eina_loader_unload_plugin(EinaLoader *self, EinaPlugin *plugin, GError **error)
{
	return FALSE;
}

G_MODULE_EXPORT GelHubSlave loader_connector = {
	"loader",
	&loader_init,
	&loader_exit
};

