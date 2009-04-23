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
#define LOG_SIGNALS 0

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
	GtkScale          *scale;
	GtkImage          *img;
	GtkLabel          *label;

	GList         *msgs;
	gint           flags;
} EinaLog;

enum {
	FLAG_NONE        = 0x00,
	FLAG_LOMO_INIT   = 0x01,
	FLAG_PLAYER_INIT = 0x10
};

enum {
	EINA_LOG_RESPONSE_CLEAR = 1,
	EINA_LOG_RESPONSE_CLOSE
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
build_player(EinaLog *self);
static gboolean
setup_lomo(EinaLog *self);
static void
set_debug_level(EinaLog *self, GelDebugLevel level);
static void
append_msg(EinaLog *self, GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer);
static void
debug_handler(GelDebugLevel level, const gchar *domain, const gchar *func, const gchar *file, gint line, const gchar *buffer, EinaLog *self);

static void
action_entry_activate_cb(GtkAction *action, EinaLog *self);
static void
dialog_response_cb(GtkWidget *w, gint response, EinaLog *self);
static gboolean
dialog_expose_event_cb(GtkWidget *w, GdkEventExpose *ev, EinaLog *self);
static void
log_level_scale_value_change_cb(GtkWidget *w, EinaLog *self);
static void
plugin_load_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
plugin_unload_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
plugin_init_cb(GelApp *app, GelPlugin *plugin, EinaLog *self);
static void
plugin_fini_cb(GelApp *app, GelPlugin *plugin, gpointer data);
static void
lomo_error_cb(LomoPlayer *lomo, GError *error, gpointer data);
#if LOG_SIGNALS
static void
lomo_signal_cb(gchar *signal);
#endif

static gboolean
log_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaLog *self = g_new0(EinaLog, 1);
	self->app    = app;
	self->plugin = plugin;
	plugin->data = self;

	// If player is loaded attach menu
	if (GEL_APP_GET_PLAYER(app))
		build_player(self);

	// If lomo is loaded attach signals
	if (GEL_APP_GET_LOMO(app))
		setup_lomo(self);
	
	g_signal_connect(app, "plugin-load",   (GCallback) plugin_load_cb,   self);
	g_signal_connect(app, "plugin-unload", (GCallback) plugin_unload_cb, self);
	g_signal_connect(app, "plugin-init",   (GCallback) plugin_init_cb,   self);
	g_signal_connect(app, "plugin-fini",   (GCallback) plugin_fini_cb,   self);

	gel_debug_add_handler((GelDebugHandler) debug_handler, self);
	return TRUE;
}

static gboolean
log_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaLog *self = GEL_PLUGIN_DATA(plugin);
	GList *iter = self->msgs;
	while (iter)
	{
		LogMsg *msg = (LogMsg *) iter->data;
		g_free(msg->domain);
		g_free(msg->func);
		g_free(msg->file);
		g_free(msg->buffer);
		g_free(msg);

		iter = iter->next;
	}
	g_list_free(self->msgs);
	g_free(self);

	return TRUE;
}

static gboolean
setup_player(EinaLog *self)
{
	set_debug_level(self, gel_get_debug_level());
	g_signal_connect(self->dialog, "delete-event",  (GCallback) gtk_widget_hide_on_delete, NULL);
	g_signal_connect(self->scale,  "value-changed", (GCallback) log_level_scale_value_change_cb, self);
	g_signal_connect(self->dialog, "response", (GCallback) dialog_response_cb, self);

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
build_player(EinaLog *self)
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
		!(self->textview = GTK_TEXT_VIEW      (gtk_builder_get_object(xml_ui, "textview"))) ||
		!(self->scale    = GTK_SCALE          (gtk_builder_get_object(xml_ui, "log-level-scale"))) ||
		!(self->img      = GTK_IMAGE          (gtk_builder_get_object(xml_ui, "log-level-image"))) ||
		!(self->label    = GTK_LABEL          (gtk_builder_get_object(xml_ui, "log-level-human-level")))
		)
	{
		gel_error(N_("Not all required widgets are valids"));
		g_object_unref(xml_ui);
		return FALSE;
	}
	g_object_unref(xml_ui);

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

	g_signal_connect(self->dialog, "expose-event", (GCallback) dialog_expose_event_cb, self);

	return TRUE;
}

static gboolean
setup_lomo(EinaLog *self)
{
	g_return_val_if_fail((self->flags & FLAG_LOMO_INIT) == 0, FALSE);

	LomoPlayer *lomo = GEL_APP_GET_LOMO(self->app);
	g_return_val_if_fail(lomo != NULL, FALSE);

#if LOG_SIGNALS
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
	
	for (i = 0; i < G_N_ELEMENTS(lomo_signals); i++)
		g_signal_connect_swapped(lomo, lomo_signals[i], (GCallback) lomo_signal_cb, (gpointer) lomo_signals[i]);
#endif

	g_signal_connect(lomo, "error", (GCallback) lomo_error_cb, NULL);
	self->flags |= FLAG_LOMO_INIT;
	return TRUE;
}

static void
set_debug_level(EinaLog *self, GelDebugLevel level)
{
	static gchar *icons[] = {
		GTK_STOCK_STOP,
		GTK_STOCK_DIALOG_ERROR,
		GTK_STOCK_DIALOG_WARNING,
		GTK_STOCK_DIALOG_INFO,
		GTK_STOCK_INFO,
		GTK_STOCK_EXECUTE,
	};
	static gchar *labels[] = {
		N_("Severe errors"),
		N_("Errors"),
		N_("Warnings"),
		N_("Information messages"),
		N_("Debug information"),
		N_("Full debug")
	};
	static gint curr_level = -1;
	g_return_if_fail((level >= 0) && (level < GEL_N_DEBUG_LEVELS));

	gel_warn("Set level to %s", labels[level]);
	if (curr_level == level)
		return;

	curr_level = level;
	gtk_image_set_from_stock(self->img, icons[level], GTK_ICON_SIZE_MENU);
	gtk_label_set_text(self->label, labels[level]);
	gtk_range_set_value((GtkRange *) self->scale, (gdouble) level);
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
		build_player(self);
	else if (g_str_equal(plugin->name, "lomo") && !(self->flags & FLAG_LOMO_INIT))
		setup_lomo(self);
}

static void
plugin_fini_cb(GelApp *app, GelPlugin *plugin, gpointer data)
{
	gel_debug("Fini plugin '%s'", PSTR(plugin));
}

#if LOG_SIGNALS
static void
lomo_signal_cb(gchar *signal)
{
	gel_debug("Lomo signal: %s", signal);
}
#endif

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

static void
dialog_response_cb(GtkWidget *w, gint response, EinaLog *self)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	switch (response)
	{
	case EINA_LOG_RESPONSE_CLEAR:
		buffer = gtk_text_view_get_buffer(self->textview);
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer,   &end);
		gtk_text_buffer_delete(buffer, &start, &end);
		break;

	case EINA_LOG_RESPONSE_CLOSE:
		gtk_widget_hide((GtkWidget *) w);
		break;

	default:
		break;
	}
}

static gboolean
dialog_expose_event_cb(GtkWidget *w, GdkEventExpose *ev, EinaLog *self)
{
	setup_player(self);
	g_signal_handlers_disconnect_by_func((GObject *) w, dialog_expose_event_cb, self);
	return FALSE;
}

static void
log_level_scale_value_change_cb(GtkWidget *w, EinaLog *self)
{
	GelDebugLevel level = CLAMP((gint) gtk_range_get_value ((GtkRange *) w), 0, GEL_N_DEBUG_LEVELS - 1);
	set_debug_level(self, level);
}

G_MODULE_EXPORT GelPlugin log_plugin = {
	GEL_PLUGIN_SERIAL,
	"log", PACKAGE_VERSION, NULL,
	NULL, NULL,

	N_("Build-in log"), N_("Build-in log"), NULL,
	
	log_init, log_fini,

	NULL, NULL, NULL
};

