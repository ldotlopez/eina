/*
 * plugins/ntfy/ntfy.c
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

#define GEL_DOMAIN "Eina::Plugin::Ntfy"
#define GEL_PLUGIN_DATA_TYPE EinaNtfy
#include <libnotify/notify.h>
#include <eina/eina-plugin.h>
#include <eina/vogon.h>
#include <plugins/ntfy/ntfy.h>

#define SETTINGS_PATH "/plugins/notify"
#define ACTION_NAME   "Notifications"
#define ACTION_PATH   "/MainMenu/PluginsPlaceholder/Notifications"

struct _EinaNtfy {
	EinaObj  parent;
	gboolean enabled;

	NotifyNotification *notification;
	GtkStatusIcon      *status_icon;

	guint vogon_merge_id;
	GtkActionGroup *ag;

	ArtSearch *search;

	LomoStream *stream;
	gchar      *basename, *body;
	GdkPixbuf  *nullcover, *cover;
};

static gboolean
ntfy_enable(EinaNtfy *self, GError **error);
static void
ntfy_disable(EinaNtfy *self);
static void
vogon_enable(EinaNtfy *self);
static void
vogon_disable(EinaNtfy *self);
static void
app_init_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self);
static void
app_fini_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self);
static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaNtfy *self);
static void
lomo_play_cb(LomoPlayer *lomo, EinaNtfy *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfy *self);
static gchar*
str_parser_cb(gchar key, LomoStream *stream);
static void
art_search_cb(Art *art, ArtSearch *search, EinaNtfy *self);
static void
action_activate_cb(GtkAction *action, EinaNtfy *self);


GEL_AUTO_QUARK_FUNC(ntfy)

G_MODULE_EXPORT gboolean
ntfy_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaNtfy *self;

	self = g_new0(EinaNtfy, 1);
	if(!eina_obj_init(EINA_OBJ(self), plugin, "ntfy", EINA_OBJ_NONE, error))
	{
		gel_error("Cannot create component");
		return FALSE;
	}

	// Load conf
	EinaConf *conf = EINA_OBJ_GET_SETTINGS(self);
	if (conf == NULL)
	{
		g_set_error(error, ntfy_quark(), EINA_NTFY_SETTINGS_ERROR,
			N_("Cannot get settings object"));
		eina_obj_fini(EINA_OBJ(self));
		return FALSE;
	}

	// Enable if needed
	if (eina_conf_get_bool(conf, SETTINGS_PATH "/enabled", TRUE))
	{
		if (!notify_is_initted() && !notify_init("eina"))
		{
			self->enabled = FALSE;
			eina_obj_fini(EINA_OBJ(self));
			return FALSE;
		}
		ntfy_enable(self, NULL);
	}
	vogon_enable(self);

	plugin->data = self;

	return TRUE;
}

G_MODULE_EXPORT gboolean
ntfy_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaNtfy *self = GEL_PLUGIN_DATA(plugin);

	vogon_disable(self);
	ntfy_disable(self);

	eina_obj_fini(EINA_OBJ(GEL_PLUGIN_DATA(plugin)));
	return TRUE;
}

static void
ntfy_sync(EinaNtfy *self)
{	
	g_return_if_fail(self->stream != NULL);
	if (self->notification)
	{
		notify_notification_close(self->notification, NULL);
		g_object_unref(self->notification);
		self->notification = NULL;
	}

	// Fill defaults
	if (!self->basename)
	{
		gchar *escape = g_uri_unescape_string(lomo_stream_get_tag(self->stream, LOMO_TAG_URI), NULL);
		self->basename = g_path_get_basename(escape);
		g_free(escape);
	}

	gchar *path = NULL;
	if ((!self->nullcover) && gel_plugin_get_resource(eina_obj_get_plugin(EINA_OBJ(self)), GEL_RESOURCE_IMAGE, "cover-default.png"))
	{
		self->nullcover = gdk_pixbuf_new_from_file_at_scale(path, 64, 64, TRUE, NULL);
		g_free(path);
	}

	// Try to fill body
	if (!self->body && lomo_stream_get_all_tags_flag(self->stream))
	{
		self->body = gel_str_parser("{{%a\n}%t}", (GelStrParserFunc) str_parser_cb, self->stream);
		if (self->body == NULL)
			self->body = g_strdup(self->basename);
	}

	if (!self->cover)
		self->search = art_search(EINA_OBJ_GET_ART(self), self->stream, (ArtFunc) art_search_cb, self);

	gchar     *body   = self->body  ? self->body  : self->basename;
	GdkPixbuf *pixbuf = self->cover ? self->cover : self->nullcover;

	if (lomo_player_get_state(EINA_OBJ_GET_LOMO(self)) != LOMO_STATE_PLAY)
		return;

	if (self->status_icon)
		self->notification = notify_notification_new_with_status_icon(N_("Playing:"), body, NULL, self->status_icon);
	else
		self->notification = notify_notification_new(N_("Playing:"), body, NULL, NULL);
	notify_notification_set_icon_from_pixbuf(self->notification, pixbuf);
	notify_notification_show(self->notification, NULL);
}

static gboolean
ntfy_enable(EinaNtfy *self, GError **error)
{
	if (self->enabled)
		return TRUE;
	
	g_signal_connect(EINA_OBJ_GET_LOMO(self), "play",     (GCallback) lomo_play_cb,     self);
	g_signal_connect(EINA_OBJ_GET_LOMO(self), "change",   (GCallback) lomo_change_cb,   self);
	g_signal_connect(EINA_OBJ_GET_LOMO(self), "all-tags", (GCallback) lomo_all_tags_cb, self);
	self->enabled = TRUE;

	return TRUE;
}

static void
ntfy_disable(EinaNtfy *self)
{
	if (!self->enabled)
		return;

	self->stream = NULL;
	if (self->search)
	{
		art_cancel(EINA_OBJ_GET_ART(self), self->search);
		self->search = NULL;
	}
	gel_free_and_invalidate(self->basename, NULL, g_free);
	gel_free_and_invalidate(self->body,     NULL, g_free);
	gel_free_and_invalidate(self->cover,    NULL, g_object_unref);

	g_signal_handlers_disconnect_by_func(EINA_OBJ_GET_LOMO(self), (GCallback) lomo_play_cb,     self);
	g_signal_handlers_disconnect_by_func(EINA_OBJ_GET_LOMO(self), (GCallback) lomo_change_cb,   self);
	g_signal_handlers_disconnect_by_func(EINA_OBJ_GET_LOMO(self), (GCallback) lomo_all_tags_cb, self);
	self->enabled = FALSE;
}

static void
vogon_enable(EinaNtfy *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	if (vogon)
	{
		g_signal_connect(eina_obj_get_app(self), "plugin-fini", (GCallback) app_fini_cb, self);
		self->status_icon = eina_vogon_get_status_icon(vogon);

		GtkUIManager *ui_mng =  eina_vogon_get_ui_manager(vogon);
		if (!ui_mng)
			return;
		const gchar *ui_xml = 
		"<ui>"
		"	<popup name='MainMenu'>"
		"		<placeholder name='PluginsPlaceholder' >"
		"			<separator />"
		"			<menuitem action='Notifications' />"
		"			<separator />"
		"		</placeholder>"
		"	</popup>"
		"</ui>";

		GtkToggleActionEntry ui_actions[] = {
			{ "Notifications", NULL, N_("Show notifications"), NULL, N_("Show notifications"), G_CALLBACK(action_activate_cb) }
		};

		self->ag = gtk_action_group_new("notify");
		gtk_action_group_add_toggle_actions(self->ag, ui_actions, G_N_ELEMENTS(ui_actions), self);

		GError *err = NULL;
		if ((self->vogon_merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_xml, -1, &err)) == 0)
		{
			gel_error("Cannot merge menu: %s", err->message);
			g_error_free(err);
			g_object_unref(self->ag);
			return;
		}

		gtk_ui_manager_insert_action_group(ui_mng, self->ag, 0);
		gtk_ui_manager_ensure_update(ui_mng);

		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_ui_manager_get_action(ui_mng, ACTION_PATH)), self->enabled);

	}
	else
	{
		g_signal_connect(eina_obj_get_app(self), "plugin-init", (GCallback) app_init_cb, self);
	}
}

static void
vogon_disable(EinaNtfy *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	g_return_if_fail(vogon != NULL);

	self->status_icon = NULL;
	if (self->vogon_merge_id == 0)
		return;
	GtkUIManager *ui_mng = eina_vogon_get_ui_manager(vogon);
	if (!ui_mng)
		return;
	gtk_ui_manager_remove_action_group(ui_mng, self->ag);
	g_object_unref(self->ag);
	gtk_ui_manager_remove_ui(ui_mng, self->vogon_merge_id);
	self->vogon_merge_id = 0;
}

static void
app_init_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self)
{
	if (!g_str_equal(plugin->name, "vogon"))
		return;
	vogon_enable(self);
	g_signal_handlers_disconnect_by_func(app, app_init_cb, self);
}

static void
app_fini_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self)
{
	if (!g_str_equal(plugin->name, "vogon"))
		return;
	vogon_disable(self);
	g_signal_handlers_disconnect_by_func(app, app_fini_cb, self);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaNtfy *self)
{
	self->stream = lomo_player_nth_stream(lomo, to);
	if (self->search)
	{
		art_cancel(EINA_OBJ_GET_ART(self), self->search);
		self->search = NULL;
	}
	gel_free_and_invalidate(self->basename, NULL, g_free);
	gel_free_and_invalidate(self->body,     NULL, g_free);
	gel_free_and_invalidate(self->cover,    NULL, g_object_unref);
}

static void
lomo_play_cb(LomoPlayer *lomo, EinaNtfy *self)
{
	if (!self->enabled)
		return;

	g_return_if_fail(self->stream != NULL);
	ntfy_sync(self);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfy *self)
{
	if (stream != self->stream)
		return;
	ntfy_sync(self);
}

static void
art_search_cb(Art *art, ArtSearch *search, EinaNtfy *self)
{
	self->search = NULL;
	gpointer *result = art_search_get_result(search);
	if (result == NULL)
		return;

	if (GDK_IS_PIXBUF(result))
	{
		self->cover = gdk_pixbuf_scale_simple((GdkPixbuf*) result, 64, 64, GDK_INTERP_BILINEAR);
		g_object_unref((GObject*) result);
	}
	else
	{
		self->cover = gdk_pixbuf_new_from_file_at_scale((gchar*) result, 64, 64, TRUE, NULL);
		g_free(result);
	}
	ntfy_sync(self);
}

static gchar*
str_parser_cb(gchar key, LomoStream *stream)
{
	switch (key)
	{
	case 't':
		return g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE));
	case 'a':
		return g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_ARTIST));
	default:
		return NULL;
	}
}

static void
action_activate_cb(GtkAction *action, EinaNtfy *self)
{
	const gchar *name = gtk_action_get_name(action);
	if (g_str_equal(ACTION_NAME, name))
	{
		gboolean active = gtk_toggle_action_get_active((GtkToggleAction*) action);
		if (active)
			ntfy_enable(self, NULL);
		else
			ntfy_disable(self);

		eina_conf_set_bool(EINA_OBJ_GET_SETTINGS(self), SETTINGS_PATH "/Notifications", self->enabled);
		gtk_toggle_action_set_active((GtkToggleAction*) action, self->enabled);
	}
}

EINA_PLUGIN_SPEC(ntfy,
	"0.0.1",
	"lomo",

	NULL,
	NULL,

	N_("Notifications plugin"),
	NULL,
	"ntfy.png",

	ntfy_init,
	ntfy_fini
);

