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

struct EinaIFaceSignalPack {
	gchar *signal;
	gpointer callback;
};

struct _EinaPluginPrivateV2 {
	gboolean    enabled;
	gchar      *plugin_name;
	gchar      *pathname;
	GModule    *module;

	EinaIFace  *iface;
	LomoPlayer *lomo;
};

typedef struct _EinaPluginPrivate
{
	gchar      *pathname;
	GModule    *mod;
	EinaIFace  *iface;
	LomoPlayer *lomo;

} _EinaPluginPrivate;

void
eina_iface_dock_init(EinaIFace *self);

/*
 * Callbacks
 */

// Watch for load of player in case it's not already loaded before us
void
on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data);

// LomoPlayer signals
#if 0
void
on_eina_iface_lomo_play(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_pause(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_stop(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_seek(LomoPlayer *lomo, gint64 old_pos, gint64 new_pos, gpointer data);
void
on_eina_iface_lomo_mute(LomoPlayer *lomo, gboolean mute, gpointer data);
void
on_eina_iface_lomo_add(LomoPlayer *lomo, LomoStream *stream, gint pos, gpointer data);
void
on_eina_iface_lomo_del(LomoPlayer *lomo, gint pos, gpointer data);
void
on_eina_iface_lomo_change(LomoPlayer *lomo, gint from, gint to, gpointer data);
void
on_eina_iface_lomo_clear(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_random(LomoPlayer *lomo, gboolean val, gpointer data);
void
on_eina_iface_lomo_repeat(LomoPlayer *lomo, gboolean val, gpointer data);
void
on_eina_iface_lomo_eos(LomoPlayer *lomo, gpointer data);
void
on_eina_iface_lomo_error(LomoPlayer *lomo, GError *err, gpointer data);
void
on_eina_iface_lomo_tag(LomoPlayer *lomo, LomoStream *stream, LomoTag tag, gpointer data);
void
on_eina_iface_lomo_all_tags(LomoPlayer *lomo, LomoStream *stream, gpointer data);
#endif

G_MODULE_EXPORT gboolean eina_iface_init
(GelHub *hub, gint *argc, gchar ***argv)
{
	EinaIFace *self;
#if 0
	gint i;
	struct EinaIFaceSignalPack signals[] = {
		{ "play",     on_eina_iface_lomo_play     },
		{ "pause",    on_eina_iface_lomo_pause    },
		{ "stop",     on_eina_iface_lomo_stop     },
		{ "seek",     on_eina_iface_lomo_seek     },
		{ "mute",     on_eina_iface_lomo_mute     },
		{ "add",      on_eina_iface_lomo_add      },
		{ "del",      on_eina_iface_lomo_del      },
		{ "change",   on_eina_iface_lomo_change   },
		{ "clear",    on_eina_iface_lomo_clear    },
		{ "random",   on_eina_iface_lomo_random   },
		{ "repeat",   on_eina_iface_lomo_repeat   },
		{ "eos",      on_eina_iface_lomo_eos      },
		{ "error",    on_eina_iface_lomo_error    },
		{ "tag",      on_eina_iface_lomo_tag      },
		{ "all-tags", on_eina_iface_lomo_all_tags },
		{ NULL, NULL }
	};
#endif

	// Create mem in hub
	self = g_new0(EinaIFace, 1);
	if (!eina_base_init((EinaBase *) self, hub, "iface", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

#if 0
	// Connect all signals of LomoPlayer
	for (i = 0; signals[i].signal != NULL; i++)
	{
		g_signal_connect(LOMO(self), signals[i].signal,
			(GCallback) signals[i].callback, self);
	}
#endif

	if ((self->player = gel_hub_shared_get(HUB(self), "player")) == NULL)
	{
		gel_warn("Player is not loaded, schuld. dock setup");
		g_signal_connect(hub, "module-load", G_CALLBACK(on_eina_iface_hub_load), self);
	}
	else
	{
		eina_iface_dock_init(self);
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

GelHub *eina_iface_get_hub(EinaIFace *self)
{
	return HUB(self);
}

LomoPlayer *eina_iface_get_lomo(EinaIFace *self)
{
	return LOMO(self);
}

EinaCover *eina_iface_get_cover(EinaIFace *self)
{
	GelHub *hub;
	EinaPlayer *player;
	
	if ((hub = HUB(self)) == NULL)
		return NULL;

	if ((player = gel_hub_shared_get(hub, "player")) == NULL)
		return NULL;

	return eina_player_get_cover(player);
}

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

EinaPluginV2 *eina_iface_load_plugin_by_path(EinaIFace *self, gchar *plugin_name, gchar *plugin_path)
{
	EinaPluginV2 *ret = NULL;
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

	ret = (EinaPluginV2 *) symbol;
	ret->priv = g_new0(EinaPluginPrivateV2, 1);
	ret->priv->plugin_name = g_strdup(plugin_name);
	ret->priv->pathname    = g_strdup(plugin_path);
	ret->priv->module      = mod;
	ret->priv->iface       = self;
	ret->priv->lomo        = gel_hub_shared_get(eina_iface_get_hub(self), "lomo");

	gel_debug("Module '%s' has been loaded", plugin_path);
	return ret;
}

EinaPluginV2 *
eina_iface_load_plugin_by_name(EinaIFace *self, gchar *plugin_name)
{
	GList *candidates, *iter;
	EinaPluginV2 *ret = NULL;

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
eina_iface_init_plugin(EinaIFace *self, EinaPluginV2 *plugin)
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
eina_iface_fini_plugin(EinaIFace *self, EinaPluginV2 *plugin)
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
eina_iface_unload_plugin(EinaIFace *self, EinaPluginV2 *plugin)
{
	g_free(plugin->priv->plugin_name);
	g_free(plugin->priv->pathname);
	g_module_close(plugin->priv->module);
	g_free(plugin->priv);
}

// --
// EinaPlugin (V1)
// --
#if 0
gboolean eina_iface_load_plugin(EinaIFace *self, gchar *plugin_name)
{
	GList *plugin_paths = NULL, *l = NULL;
	gchar *plugin_filename = NULL;
	GModule *mod = NULL;
	gchar *plugin_symbol = NULL; // Name of the symbol to lookup
	gpointer plugin_init = NULL; // The symbol it self
	EinaPlugin *plugin = NULL;   // EinaPlugin struct returned by plugin

	plugin_paths = eina_iface_get_plugin_paths();

	plugin_symbol = g_strdup_printf("%s_init", plugin_name);
	l = plugin_paths;
	while (l) {

		gchar *plugin_dirname = g_build_filename((gchar *) l->data, plugin_name, NULL);
		plugin_filename = g_module_build_path(plugin_dirname, plugin_name);
		g_free(plugin_dirname);
		eina_iface_debug("Testing %s", plugin_filename);
		eina_iface_load_plugin_by_path(self, plugin_name, plugin_filename);

		if ((mod = g_module_open(plugin_filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)) == NULL)
		{
			g_free(plugin_filename);
			l = l->next;
			continue;
		}

		if (!g_module_symbol(mod, plugin_symbol, &plugin_init))
		{
			// Not found
			gel_warn("Found matching plugin for '%s' at '%s' but has no symbol '%s'",
				plugin_name,
				plugin_filename,
				plugin_symbol);
			g_module_close(mod);
		}
		else
		{
			// Found
			break;
		}

		g_free(plugin_filename);
		break;

		l = l->next;
	}
	g_free(plugin_symbol);

	if (plugin_init != NULL) {
		plugin = ((EinaPluginInitFunc)plugin_init)(HUB(self), self);
		if (plugin != NULL)
		{
			// Fill EinaPluginPrivate
			gel_info("Loading plugin %s for %s", plugin_filename, plugin_name);
			plugin->priv->pathname = plugin_filename;
			plugin->priv->mod      = mod;
			plugin->priv->iface    = self;
			plugin->priv->lomo     = gel_hub_shared_get(GEL_HUB(HUB(self)), "lomo");

			self->plugins = g_list_append(self->plugins, plugin);

			if (plugin->dock_widget != NULL)
				eina_iface_dock_add(self, (gchar *) plugin->name, plugin->dock_widget, plugin->dock_label_widget);
		}
	}

	g_list_foreach(plugin_paths, (GFunc) g_free, NULL);
	g_list_free(plugin_paths);

	return FALSE;
}

gchar *
eina_iface_build_plugin_filename(gchar *plugin_name, gchar *filename)
{
	return  g_build_filename(g_get_home_dir(), ".eina", "plugins", plugin_name, filename, NULL);
}

gchar *
eina_iface_get_plugin_dir(gchar *plugin_name)
{
	return  g_build_filename(g_get_home_dir(), ".eina", "plugins", plugin_name, NULL);
}

gchar *
eina_iface_plugin_resource_get_pathname(EinaPlugin *plugin, gchar *resource)
{
	gchar *ret;
	GList *plugin_paths, *iter;

	gboolean found = FALSE;
	gchar *plugin_dirname = NULL;

	if (plugin == NULL) 
	{
		gel_error("Invalid plugin");
		return NULL;
	}

	if ((plugin->priv == NULL) || (plugin->priv->pathname == NULL))
	{
		gel_warn("Oops, plugin is not initialize yet. Using heuristic mode to guess resource pathname");
		iter = plugin_paths = eina_iface_get_plugin_paths();
		while (iter && !found)
		{
			ret = g_build_filename(iter->data, plugin->name, resource, NULL);
			if (g_file_test(ret, G_FILE_TEST_IS_REGULAR))
			{
				found = TRUE;
				break;
			}
			g_free(ret);

			iter = iter->next;
		}
		gel_glist_free(plugin_paths, (GFunc) g_free, NULL);

		if (found)
		{
			gel_info("Found resource via heuristic method at '%s'", ret);
			return ret;
		}
		else
		{
			gel_error("Heuristic mode: epic fail");
			return NULL;
		}
	}

	else
	{
		plugin_dirname = g_path_get_dirname(plugin->priv->pathname);
		if (plugin_dirname == NULL)
			return NULL;

		ret = g_build_filename(plugin_dirname, resource, NULL);
		gel_info("RET: '%s'", ret);
		g_free(plugin_dirname);
		return ret;
	}
}
#endif

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

	for (i = 0; i < gtk_notebook_get_n_pages(self->dock); i++)
		gtk_notebook_remove_page(self->dock, i);
	self->dock_items = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
	gtk_notebook_set_show_tabs(self->dock, FALSE);

	gel_info("Dock initialized");
}

gboolean eina_iface_dock_add(EinaIFace *self, gchar *id, GtkWidget *dock_widget, GtkWidget *label)
{
	if (!self->dock || (g_hash_table_lookup(self->dock_items, id) != NULL))
	{
		return FALSE;
	}

	if (gtk_notebook_append_page(self->dock, dock_widget, label) == -1)
	{
		gel_error("Cannot add widget to dock");
		return FALSE;
	}
	gel_info("Added dock '%s'", id);
	// g_object_ref(G_OBJECT(dock_widget));
	g_hash_table_insert(self->dock_items, g_strdup(id), (gpointer) dock_widget);

	if (gtk_notebook_get_n_pages(self->dock) > 1)
		gtk_notebook_set_show_tabs(self->dock, TRUE);

	return TRUE;
}

gboolean eina_iface_dock_remove(EinaIFace *self, gchar *id)
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
eina_iface_dock_switch(EinaIFace *self, gchar *id)
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

void on_eina_iface_hub_load(GelHub *hub, const gchar *modname, gpointer data)
{
	EinaIFace  *self = (EinaIFace *) data;

	if (!g_str_equal(modname, "player"))
		return;

	if ((self->player = gel_hub_shared_get(hub, "player")) == NULL)
		return;

	eina_iface_dock_init(self);
}

G_MODULE_EXPORT GelHubSlave iface_connector = {
	"iface",
	&eina_iface_init,
	&eina_iface_exit
};

