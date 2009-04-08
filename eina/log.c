/*
 * eina/log.c
 *
 * Copyright (C) 2004-2009 Eina
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

#define GEL_DOMAIN "Eina::Log"
#define GEL_PLUGIN_DATA_TYPE EinaLog

#include <config.h>
#include <eina/eina-plugin.h>
#include <eina/player.h>

#ifdef PSTR
#undef PSTR
#endif
#define PSTR(p) gel_plugin_stringify(p)

typedef struct {
	GelApp    *app;
	GelPlugin *plugin;

	guint           merge_id;
	GtkActionGroup *ag;

	GtkDialog         *dialog;
	GtkScrolledWindow *sc;
	GtkTextView       *textview;

	GList         *msgs;
	gint           flags;
} EinaLog;

enum {
	FLAG_NONE        = 0x00,
	FLAG_LOMO_INIT   = 0x01,
	FLAG_PLAYER_INIT = 0x10
};

typedef struct {
	GelDebugLevel level;
	gchar *domain;
	gchar *func;
	gchar *file;
	gint   line;
	gchar *buffer;
} LogMsg;

static gboolean
setup_player(EinaLog *self);
static gboolean
setup_lomo(EinaLog *self);
static void
append_msg(EinaLog *self, GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer);
static void
debug_handler(GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer, EinaLog *self);

static void
action_entry_activate_cb(GtkAction *action, EinaLog *self);
static void
plugin_load_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
plugin_unload_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
plugin_init_cb(GelApp *app, GelPlugin *plugin, EinaLog *self);
static void
plugin_fini_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
lomo_signal_cb(gchar *signal);
static void
lomo_error_cb(LomoPlayer *lomo, GError *error, gpointer data);


static gboolean
log_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaLog *self = g_new0(EinaLog, 1);
	self->app    = app;
	self->plugin = plugin;

	// If player is loaded attach menu
	if (GEL_APP_GET_PLAYER(app))
		setup_player(self);

	// If lomo is loaded attach signals
	if (GEL_APP_GET_LOMO(app))
		setup_lomo(self);
	
	g_signal_connect(app, "plugin-load",   (GCallback) plugin_load_cb,   self);
	g_signal_connect(app, "plugin-unload", (GCallback) plugin_unload_cb, self);
	g_signal_connect(app, "plugin-init",   (GCallback) plugin_init_cb,   self);
	g_signal_connect(app, "plugin-fini",   (GCallback) plugin_fini_cb,   self);

	gel_debug_add_handler((GelDebugHandler) debug_handler, self);
	gel_set_debug_level(GEL_DEBUG_LEVEL_VERBOSE);
	return TRUE;
}
static gboolean
log_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	return TRUE;
}

static gboolean
setup_player(EinaLog *self)
{
	g_return_val_if_fail((self->flags & FLAG_PLAYER_INIT) == 0, FALSE);

	gchar *xml_path = gel_app_resource_get_pathname(GEL_APP_RESOURCE_UI, "log.ui");
	GtkBuilder *xml_ui = gtk_builder_new();
	GError *error = NULL;
	if (gtk_builder_add_from_file(xml_ui, xml_path, &error) == 0)
	{
		gel_warn("Cannot load UI from %s: %s", xml_path, error->message);
		g_error_free(error);
		g_free(xml_path);
		return FALSE;
	}
	g_free(xml_path);

	if (
		!(self->dialog   = GTK_DIALOG         (gtk_builder_get_object(xml_ui, "main-window"))) ||
		!(self->sc       = GTK_SCROLLED_WINDOW(gtk_builder_get_object(xml_ui, "scrolledwindow"))) ||
		!(self->textview = GTK_TEXT_VIEW      (gtk_builder_get_object(xml_ui, "textview")))
		)
	{
		gel_error(N_("Not all required widgets are valids"));
		g_object_unref(xml_ui);
		return FALSE;
	}
	g_object_unref(xml_ui);
	g_signal_connect(self->dialog, "response",     (GCallback) gtk_widget_hide_on_delete, NULL);
	g_signal_connect(self->dialog, "delete-event", (GCallback) gtk_widget_hide_on_delete, NULL);

	GtkUIManager *uimng = eina_player_get_ui_manager(GEL_APP_GET_PLAYER(self->app));
	self->merge_id = gtk_ui_manager_add_ui_from_string(uimng,
		"<ui>"
		"<menubar name=\"MainMenuBar\">"
		"<menu name=\"Help\" action=\"HelpMenu\" >"
		"<menuitem name=\"Debug\" action=\"Debug\" />"
		"</menu>"
		"</menubar>"
		"</ui>",
		-1, &error);
	if (self->merge_id == 0)
	{
		gel_warn("Cannot create menu item: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	GtkActionEntry action_entries[] = {
		{ "Debug", NULL, N_("Debug window"),
		NULL, NULL, G_CALLBACK(action_entry_activate_cb) }
	};

	self->ag = gtk_action_group_new("debug");
	gtk_action_group_add_actions(self->ag, action_entries, G_N_ELEMENTS(action_entries), self);
	gtk_ui_manager_insert_action_group(uimng, self->ag, 1);
	gtk_ui_manager_ensure_update(uimng);

	GList *iter = g_list_last(self->msgs);
	while (iter)
	{
		LogMsg *msg = (LogMsg *) iter->data;
		append_msg(self, msg->level, msg->domain, msg->func, msg->file, msg->line, msg->buffer);

		g_free(msg->domain);
		g_free(msg->func);
		g_free(msg->file);
		g_free(msg->buffer);
		g_free(msg);

		iter = iter->prev;
	}
	gel_free_and_invalidate(self->msgs, NULL, g_list_free);

	self->flags |= FLAG_PLAYER_INIT;
	return TRUE;
}

static gboolean
setup_lomo(EinaLog *self)
{
	g_return_val_if_fail((self->flags & FLAG_LOMO_INIT) == 0, FALSE);
	const gchar *lomo_signals[] =
	{
		"play",
		"pause",
		"stop",
		"seek",
		"mute",
		"insert",
		"remove",
		"change",
		"clear",
		"repeat",
		"random",
		"eos",
		"error",
		// "tag",
		// "all_tags"
	};
	gint i;
	
	LomoPlayer *lomo = GEL_APP_GET_LOMO(self->app);
	g_return_val_if_fail(lomo != NULL, FALSE);

	for (i = 0; i < G_N_ELEMENTS(lomo_signals); i++)
		g_signal_connect_swapped(lomo, lomo_signals[i], (GCallback) lomo_signal_cb, (gpointer) lomo_signals[i]);

	g_signal_connect(lomo, "error", (GCallback) lomo_error_cb, NULL);
	self->flags |= FLAG_LOMO_INIT;
	return TRUE;
}

static void
append_msg(EinaLog *self, GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer)
{
	g_return_if_fail(self->textview != NULL);

	GtkTextBuffer *b = gtk_text_view_get_buffer(self->textview);
	gchar *text = g_strdup_printf("[%s] [%s() %s:%d] %s\n", domain, func, file, line, buffer);
	gtk_text_buffer_insert_at_cursor(b, text, g_utf8_strlen(text, -1));
	g_free(text);

	GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(self->sc);
	gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
	gtk_scrolled_window_set_vadjustment(self->sc, adj);
}

static void
debug_handler(GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer, EinaLog *self)
{
	if (self->dialog)
	{
		append_msg(self, level, domain, func, file, line, buffer);
	}
	else
	{
		LogMsg *msg = g_new0(LogMsg, 1);
		msg->level  = level;
		msg->domain = g_strdup(domain);
		msg->func   = g_strdup(func);
		msg->file   = g_strdup(file);
		msg->line   = line;
		msg->buffer = g_strdup(buffer);
		self->msgs = g_list_prepend(self->msgs, msg);
	}
}

static void
plugin_load_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Load plugin '%s'", PSTR(plugin));
}

static void
plugin_unload_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Unload plugin '%s'", PSTR(plugin));
}

static void
plugin_init_cb(GelApp *app, GelPlugin *plugin, EinaLog *self)
{
	gel_debug("Init plugin '%s'", PSTR(plugin));

	if (g_str_equal(plugin->name, "player") && !(self->flags & FLAG_PLAYER_INIT))
		setup_player(self);
	else if (g_str_equal(plugin->name, "lomo") && !(self->flags & FLAG_LOMO_INIT))
		setup_lomo(self);
}

static void
plugin_fini_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Fini plugin '%s'", PSTR(plugin));
}

static void
lomo_signal_cb(gchar *signal)
{
	gel_debug("Lomo signal: %s", signal);
}

static void
lomo_error_cb(LomoPlayer *lomo, GError *error, gpointer data)
{
	gel_debug("Lomo error: %s", error->message);
}

static void
action_entry_activate_cb(GtkAction *action, EinaLog *self)
{
	 const gchar *name = gtk_action_get_name(action);
	 if (!g_str_equal(name, "Debug"))
	 	return;

	if (self->dialog == NULL)
		gel_warn(N_("Cannot not shot dialog, it doest exists"));
	else
		gtk_widget_show((GtkWidget *) self->dialog);
}

G_MODULE_EXPORT GelPlugin log_plugin = {
	GEL_PLUGIN_SERIAL,
	"log", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in log"), N_("Build-in log"), NULL,
	
	log_init, log_fini,

	NULL, NULL, NULL
};

