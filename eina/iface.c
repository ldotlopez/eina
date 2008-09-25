#define GEL_DOMAIN "Eina::IFace"

#include <gmodule.h>
#include <gel/gel.h>
#include "base.h"
#include "iface.h"
#include "player.h"

struct _EinaIFace {
	EinaBase     parent;
	EinaPlayer  *player;
	GtkNotebook *dock;
	GHashTable  *dock_items;
	GList       *plugins;
};

struct _EinaPluginPrivate {
	gboolean    enabled;
	gchar      *plugin_name;
	gchar      *pathname;
	GModule    *module;

	EinaIFace  *iface;
	LomoPlayer *lomo;
};

EinaPlugin*
eina_iface_load_plugin_by_name(EinaIFace *self, gchar* plugin_name);
EinaPlugin*
eina_iface_load_plugin_by_path(EinaIFace *self, gchar *plugin_name, gchar *plugin_path);

gboolean
eina_iface_init_plugin(EinaIFace *self, EinaPlugin *plugin);
gboolean
eina_iface_fini_plugin(EinaIFace *self, EinaPlugin *plugin);

void
eina_iface_dock_init(EinaIFace *self);
static void
eina_iface_dock_page_signal_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaIFace *self);

/*
 * Callbacks
 */

// Watch for load of player in case it's not already loaded before us
void
on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data);

G_MODULE_EXPORT gboolean eina_iface_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaIFace *self;
	EinaPlugin *plugin;

	// Create mem in hub
	self = g_new0(EinaIFace, 1);
	if (!eina_base_init((EinaBase *) self, hub, "iface", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	if ((self->player = gel_hub_shared_get(HUB(self), "player")) == NULL)
	{
		gel_warn("Player is not loaded, schuld. dock setup");
		g_signal_connect(hub, "module-load", G_CALLBACK(on_eina_iface_hub_load), self);
	}
	else
	{
		eina_iface_dock_init(self);
		if ((plugin = eina_iface_load_plugin_by_name(self, "coverplus")) != NULL)
			eina_iface_init_plugin(self, plugin);
		if ((plugin = eina_iface_load_plugin_by_name(self, "recently")) != NULL)
			eina_iface_init_plugin(self, plugin);

	}

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_iface_exit
(gpointer data)
{
	EinaIFace *self = (EinaIFace *) data;

	eina_base_fini((EinaBase *) self);
	return TRUE;
}

// --
// Functions needed to access internal and deeper elements
// --
GelHub *eina_iface_get_hub(EinaIFace *self)
{
	return HUB(self);
}

LomoPlayer *eina_iface_get_lomo(EinaIFace *self)
{
	return LOMO(self);
}

EinaIFace *eina_plugin_get_iface(EinaPlugin *plugin)
{
	return plugin->priv->iface;
}

// --
// Plugin handling
// --
GList *
eina_iface_get_plugin_paths(void)
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

GList *eina_iface_lookup_plugin(EinaIFace *self, gchar *plugin_name)
{
	GList *search_paths, *iter;
	GList *ret = NULL;

	gel_debug("Searching for plugin '%s'", plugin_name);
	iter = search_paths = eina_iface_get_plugin_paths();
	while (iter)
	{
		GList *plugins, *iter2;

		gel_debug(" Inspecting '%s'", (gchar *) iter->data);
		iter2 = plugins = gel_dir_read((gchar *) iter->data, TRUE, NULL);
		while (iter2)
		{
			gchar *plugin_pathname = (gchar *) iter2->data;
			gchar *plugin_basename = g_path_get_basename(plugin_pathname);

			if (g_str_equal(plugin_basename, plugin_name))
			{
				gchar *modname = g_module_build_path(plugin_pathname, plugin_name);
				gel_debug("  Found candidate '%s'", modname);
				ret = g_list_append(ret, modname);
			}
			g_free(plugin_basename);

			iter2 = iter2->next;
		}
		gel_glist_free(plugins, (GFunc) g_free, NULL);

		iter = iter->next;
	}
	gel_glist_free(search_paths, (GFunc) g_free, NULL);

	return g_list_reverse(ret);
}

EinaPlugin *eina_iface_load_plugin_by_path(EinaIFace *self, gchar *plugin_name, gchar *plugin_path)
{
	EinaPlugin *ret = NULL;
	GModule    *mod;
	gchar      *symbol_name;
	gpointer    symbol;

	if (!g_module_supported())
	{
		gel_error("Module loading is NOT supported on this platform");
		return NULL;
	}

	if ((mod = g_module_open(plugin_path, G_MODULE_BIND_LOCAL)) == NULL)
	{
		gel_debug("'%s' is not loadable", plugin_path);
		return NULL;
	}

	symbol_name = g_strconcat(plugin_name, "_plugin", NULL);

	if (!g_module_symbol(mod, symbol_name, &symbol))
	{
		gel_debug("Cannot find symbol '%s' in '%s'", symbol_name, plugin_path);
		g_free(symbol_name);
		g_module_close(mod);
		return NULL;
	}

	ret = (EinaPlugin *) symbol;
	ret->priv = g_new0(EinaPluginPrivate, 1);
	ret->priv->plugin_name = g_strdup(plugin_name);
	ret->priv->pathname    = g_strdup(plugin_path);
	ret->priv->module      = mod;
	ret->priv->iface       = self;
	ret->priv->lomo        = gel_hub_shared_get(eina_iface_get_hub(self), "lomo");

	gel_debug("Module '%s' has been loaded", plugin_path);
	return ret;
}

EinaPlugin *
eina_iface_load_plugin_by_name(EinaIFace *self, gchar *plugin_name)
{
	GList *candidates, *iter;
	EinaPlugin *ret = NULL;

	iter = candidates = eina_iface_lookup_plugin(self, plugin_name);
	gel_debug("Got %d candidates for plugin '%s'", g_list_length(candidates), plugin_name);
	while (iter && !ret)
	{
		ret = eina_iface_load_plugin_by_path(self, plugin_name, (gchar *) iter->data);
		iter = iter->next;
	}

	gel_glist_free(candidates, (GFunc) g_free, NULL);
	return ret;
}

gboolean
eina_iface_init_plugin(EinaIFace *self, EinaPlugin *plugin)
{
	GError *err = NULL;

	if (!plugin->init(plugin, &err))
	{
		gel_error("Plugin %s cannot be initialized: %s",
			plugin->priv->plugin_name,
			(err != NULL) ? err->message : "No error");
		return FALSE;
	}
	
	self->plugins = g_list_append(self->plugins, plugin);
	plugin->priv->enabled = TRUE;
	return TRUE;
}

gboolean
eina_iface_fini_plugin(EinaIFace *self, EinaPlugin *plugin)
{
	GError *err = NULL;

	if (!plugin->fini(plugin, &err))
	{
		gel_error("Plugin %s cannot be finalized: %s",
			plugin->priv->plugin_name,
			(err != NULL) ? err->message : "No error");
		return FALSE;
	}
	
	plugin->priv->enabled = FALSE;
	self->plugins = g_list_remove(self->plugins, plugin);

	return TRUE;
}

void
eina_iface_unload_plugin(EinaIFace *self, EinaPlugin *plugin)
{
	g_free(plugin->priv->plugin_name);
	g_free(plugin->priv->pathname);
	g_module_close(plugin->priv->module);
	g_free(plugin->priv);
}

// --
// Dock management
// --
void eina_iface_dock_init(EinaIFace *self)
{
	gint i;

	if (!self->player || self->dock)
		return;

	if ((self->dock = GTK_NOTEBOOK(W(self->player, "dock-notebook"))) == NULL)
	{
		gel_warn("Cannot get dock, disabled");
	}
	gtk_widget_realize(GTK_WIDGET(self->dock));

	g_signal_connect(self->dock, "page-added",
	G_CALLBACK(eina_iface_dock_page_signal_cb), self);
	g_signal_connect(self->dock, "page-removed",
	G_CALLBACK(eina_iface_dock_page_signal_cb), self);
	g_signal_connect(self->dock, "page-reordered",
	G_CALLBACK(eina_iface_dock_page_signal_cb), self);

	for (i = 0; i < gtk_notebook_get_n_pages(self->dock); i++)
		gtk_notebook_remove_page(self->dock, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	gtk_notebook_set_show_tabs(self->dock, FALSE);


	gel_info("Dock initialized");
}

gboolean eina_iface_dock_add_item(EinaIFace *self, gchar *id, GtkWidget *label, GtkWidget *dock_widget)
{
	if (!self->dock || (g_hash_table_lookup(self->dock_items, id) != NULL))
	{
		return FALSE;
	}

	g_hash_table_insert(self->dock_items, g_strdup(id), (gpointer) dock_widget);
	if (gtk_notebook_append_page(self->dock, dock_widget, label) == -1)
	{
		g_hash_table_remove(self->dock_items, id);
		gel_error("Cannot add widget to dock");
		return FALSE;
	}
	gel_info("Added dock '%s'", id);
	gtk_notebook_set_tab_reorderable(self->dock, dock_widget, TRUE);

	if (gtk_notebook_get_n_pages(self->dock) > 1)
		gtk_notebook_set_show_tabs(self->dock, TRUE);

	return TRUE;
}

gboolean eina_iface_dock_remove_item(EinaIFace *self, gchar *id)
{
	GtkWidget *dock_item;

	if (!self->dock || ((dock_item = g_hash_table_lookup(self->dock_items, id)) == NULL))
	{
		return FALSE;
	}

	gtk_notebook_remove_page(GTK_NOTEBOOK(self->dock), gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item));

	if (gtk_notebook_get_n_pages(self->dock) <= 1)
		gtk_notebook_set_show_tabs(self->dock, FALSE);

	return g_hash_table_remove(self->dock_items, id);
}

gboolean
eina_iface_dock_switch_item(EinaIFace *self, gchar *id)
{
	gint page_num;
	GtkWidget *dock_item;

	dock_item = g_hash_table_lookup(self->dock_items, (gpointer) id);
	if (dock_item == NULL)
	{
		gel_error("Cannot find dock widget '%s'", id);
		return FALSE;
	}

	page_num = gtk_notebook_page_num(GTK_NOTEBOOK(self->dock), dock_item);
	if (page_num == -1)
	{
		gel_error("Dock item %s is not in dock", id);
		return FALSE;
	}
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->dock), page_num);
	return TRUE;
}

static void eina_iface_dock_page_signal_cb_aux(gpointer k, gpointer v, gpointer data)
{
	GList *list       = (GList *) data;
	GList *p;

	p = g_list_find(list, v);
	if (p == NULL)
		return;
	
	p->data = k;
}

static void
eina_iface_dock_page_signal_cb(GtkNotebook *w, GtkWidget *widget, guint n, EinaIFace *self)
{
	gint n_tabs, i;
	GList *dock_items = NULL;
	GString *output = g_string_new(NULL);

	n_tabs = gtk_notebook_get_n_pages(w);
	for (i = (n_tabs - 1); i >= 0; i--)
	{
		dock_items = g_list_prepend(dock_items, gtk_notebook_get_nth_page(w, i));
	}

	g_hash_table_foreach(self->dock_items, eina_iface_dock_page_signal_cb_aux, dock_items);
	for (i = 0; i < n_tabs; i++)
	{
		output = g_string_append(output, (gchar *) g_list_nth_data(dock_items, i));
		if ((i+1) < n_tabs)
			output = g_string_append_c(output, ',');
	}
	gel_warn("Set order: %s", output->str);
	g_string_free(output, TRUE);
}

// --
// Cover management
// --
EinaCover*
eina_plugin_get_player_cover(EinaPlugin *plugin)
{
	EinaCover  *cover;
	EinaPlayer *player;
	
	player = gel_hub_shared_get(eina_iface_get_hub(eina_plugin_get_iface(plugin)), "player");
	if (player == NULL)
		return NULL;

	cover = eina_player_get_cover(player);
	if (cover == NULL)
		return NULL;

	return cover;
}

void
eina_plugin_cover_add_backend(EinaPlugin *plugin, gchar *id,
    EinaCoverBackendFunc search, EinaCoverBackendCancelFunc cancel)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);
	if (cover == NULL)
	{
		gel_warn("Unable to access cover");
		return;
	}
	eina_cover_add_backend(cover, id, search, cancel, plugin);
}

void
eina_plugin_cover_remove_backend(EinaPlugin *plugin, gchar *id)
{
	EinaCover *cover = eina_plugin_get_player_cover(plugin);
	if (cover == NULL)
	{
		gel_warn("Unable to access cover");
		return;
	}
	eina_cover_remove_backend(cover, id);
}


void on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data)
{
	EinaIFace  *self = (EinaIFace *) data;

	if (!g_str_equal(modname, "player"))
		return;

	if ((self->player = gel_hub_shared_get(hub, "player")) == NULL)
		return;

	eina_iface_dock_init(self);
}

// --
// LomoEvents handling
// --
void
eina_plugin_attach_events(EinaPlugin *plugin, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;

	va_start(p, plugin);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			g_signal_connect(EINA_PLUGIN_LOMO(plugin), signal,
			callback, plugin);
		signal = va_arg(p, gchar*);
	}
	va_end(p);
}

void
eina_plugin_deattach_events(EinaPlugin *plugin, ...)
{
	va_list p;
	gchar *signal;
	gpointer callback;

	va_start(p, plugin);
	signal = va_arg(p, gchar*);
	while (signal != NULL)
	{
		callback = va_arg(p, gpointer);
		if (callback)
			 g_signal_handlers_disconnect_by_func(EINA_PLUGIN_LOMO(plugin), callback, plugin);
		signal = va_arg(p, gchar*);
	}
	va_end(p);
}

// --
// Utilities for plugins
// --
gchar *
eina_plugin_build_resource_path(EinaPlugin *plugin, gchar *resource)
{
	gchar *dirname, *ret;
	dirname = g_path_get_dirname(plugin->priv->pathname);

	ret = g_build_filename(dirname, resource, NULL);
	g_free(dirname);

	return ret;
}

gchar *
eina_plugin_build_userdir_path (EinaPlugin *plugin, gchar *path)
{
	return g_build_filename(g_get_home_dir(), ".eina", plugin->priv->plugin_name, path, NULL);
}

G_MODULE_EXPORT GelHubSlave iface_connector = {
	"iface",
	&eina_iface_init,
	&eina_iface_exit
};

