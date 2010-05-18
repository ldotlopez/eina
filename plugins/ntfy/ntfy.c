/*
 * plugins/ntfy/ntfy.c
 *
 * Copyright (C) 2004-2010 Eina
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

#define SETTINGS_PATH "/ntfy"
#define ACTION_NAME   "Notifications"
#define ACTION_PATH   "/Main/PluginsPlaceholder/Notifications"

struct _EinaNtfy {
	EinaObj  parent;
	gboolean        enabled;

	gboolean        vogon_enabled;
	GtkActionGroup *vogon_ag;
	guint           vogon_merge_id;

	GtkStatusIcon *status_icon;
	
	NotifyNotification *ntfy;

	LomoStream *stream;
	gboolean all_tags;
	gboolean got_cover;
	GdkPixbuf *cover;
	GdkPixbuf *default_cover;

	ArtSearch *search;
};

// Init/fini plugin
gboolean
ntfy_init(GelApp *app, GelPlugin *plugin, GError **error);
gboolean
ntfy_fini(GelApp *app, GelPlugin *plugin, GError **error);

// Mini API
static gboolean
ntfy_enable(EinaNtfy *self, GError **error);
static void
ntfy_disable(EinaNtfy *self);
static gboolean
vogon_enable(EinaNtfy *self);
static gboolean
vogon_disable(EinaNtfy *self);

static void
ntfy_reset(EinaNtfy *self);
static void
ntfy_sync(EinaNtfy *self);

// Callback
static void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self);
static void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self);
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfy *self);
static void
art_search_cb(Art *art, ArtSearch *search, EinaNtfy *self);
static void
action_activate_cb(GtkAction *action, EinaNtfy *self);

GEL_AUTO_QUARK_FUNC(ntfy)

// ------------------
// Init / fini plugin
// ------------------

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

	// Enable if needed (by default on)
	if (eina_conf_get_bool(conf, SETTINGS_PATH "/enabled", TRUE))
	{
		if (!ntfy_enable(self, error))
		{
			ntfy_fini(app, plugin, NULL);
			return FALSE;
		}
	}

	// Try to enable vogon integration
	GelPlugin *vogon = gel_app_get_plugin_by_name(app, "vogon");
	if (vogon && gel_plugin_is_enabled(vogon))
		if (!vogon_enable(self))
			gel_warn("Cannot enable vogon integration");

	g_signal_connect(app, "plugin-init", (GCallback) app_plugin_init_cb, self);
	g_signal_connect(app, "plugin-fini", (GCallback) app_plugin_init_cb, self);

	gel_plugin_set_data(plugin, self);
	return TRUE;
}

G_MODULE_EXPORT gboolean
ntfy_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaNtfy *self = GEL_PLUGIN_DATA(plugin);

	g_signal_handlers_disconnect_by_func(app, app_plugin_init_cb, self);
	g_signal_handlers_disconnect_by_func(app, app_plugin_fini_cb, self);

	// Try to disable vogon integration
	GelPlugin *vogon = gel_app_get_plugin_by_name(app, "vogon");
	if (vogon && gel_plugin_is_enabled(vogon))
		if (!vogon_disable(self))
			gel_warn("Cannot disable vogon integration");

	ntfy_disable(self);
	eina_obj_fini((EinaObj *) self);

	return TRUE;
}

static gboolean
ntfy_enable(EinaNtfy *self, GError **error)
{
	if (self->enabled)
		return TRUE;
	
	if (!notify_is_initted())
		notify_init("Eina");

	self->enabled = TRUE;
	self->ntfy = notify_notification_new(N_("Now playing"), NULL, NULL, NULL);

	g_signal_connect_swapped(eina_obj_get_lomo(self), "play",     (GCallback) ntfy_sync, self);
	g_signal_connect_swapped(eina_obj_get_lomo(self), "change",   (GCallback) ntfy_reset, self);
	g_signal_connect(eina_obj_get_lomo(self), "all-tags", (GCallback) lomo_all_tags_cb, self);

	gchar *path = NULL;
	if (!(path = gel_resource_locate(GEL_RESOURCE_IMAGE, "cover-default.png")) ||
	    !(self->default_cover = gdk_pixbuf_new_from_file_at_scale(path, 64, 64, TRUE, NULL)))
	{
		gel_error(N_("Cannot load resource %s. Cover art will be disabled."), "cover-default.png");
	}

	gel_free_and_invalidate(path, NULL, g_free);

	return TRUE;
}

static void
ntfy_disable(EinaNtfy *self)
{
	if (!self->enabled)
		return;
	self->enabled = FALSE;

	g_signal_handlers_disconnect_by_func(eina_obj_get_lomo(self), (GCallback) ntfy_reset, self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_lomo(self), (GCallback) ntfy_sync, self);
	g_signal_handlers_disconnect_by_func(eina_obj_get_lomo(self), (GCallback) lomo_all_tags_cb, self);

	self->stream = NULL;
	self->all_tags  = FALSE;
	self->got_cover = FALSE;

	gel_free_and_invalidate(self->ntfy,  NULL, g_object_unref);
	gel_free_and_invalidate(self->cover, NULL, g_object_unref);
	gel_free_and_invalidate(self->default_cover, NULL, g_object_unref);

	if (notify_is_initted())
		notify_uninit();

	if (self->search)
	{
		art_cancel(EINA_OBJ_GET_ART(self), self->search);
		self->search = NULL;
	}
}

static gboolean
vogon_enable(EinaNtfy *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	g_return_val_if_fail(vogon != NULL, FALSE);

	if (self->vogon_enabled)
		return TRUE;
	self->vogon_enabled = TRUE;

	// Get status icon
	self->status_icon = eina_vogon_get_status_icon(vogon);

	// Gel UI Manager
	GtkUIManager *ui_mng =  eina_vogon_get_ui_manager(vogon);
	if (!ui_mng)
	{
		vogon_disable(self);
		return FALSE;
	}

	// Add actions
	GtkToggleActionEntry ui_actions[] =
	{
		{ "Notifications", NULL, N_("Show notifications"), NULL, N_("Show notifications"), G_CALLBACK(action_activate_cb) }
	};
	self->vogon_ag = gtk_action_group_new("notify");
	gtk_action_group_add_toggle_actions(self->vogon_ag, ui_actions, G_N_ELEMENTS(ui_actions), self);

	// Merge UI
	const gchar *ui_xml = 
	"<ui>"
	"	<popup name='Main'>"
	"		<placeholder name='PluginsPlaceholder' >"
	"			<separator />"
	"			<menuitem action='Notifications' />"
	"			<separator />"
	"		</placeholder>"
	"	</popup>"
	"</ui>";

	GError *err = NULL;
	if ((self->vogon_merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_xml, -1, &err)) == 0)
	{
		gel_error("Cannot merge menu: %s", err->message);
		g_error_free(err);
		vogon_disable(self);
		return FALSE;
	}

	// Add action group
	gtk_ui_manager_insert_action_group(ui_mng, self->vogon_ag, 0);
	gtk_ui_manager_ensure_update(ui_mng);

	// Set state
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_ui_manager_get_action(ui_mng, ACTION_PATH)), self->enabled);
	notify_notification_attach_to_status_icon(self->ntfy, self->status_icon);

	return TRUE;
}

static gboolean
vogon_disable(EinaNtfy *self)
{
	EinaVogon *vogon = EINA_OBJ_GET_VOGON(self);
	g_return_val_if_fail(vogon != NULL, FALSE);

	notify_notification_attach_to_status_icon(self->ntfy, NULL);

	GtkUIManager *ui_mng = eina_vogon_get_ui_manager(vogon);
	g_return_val_if_fail(ui_mng != NULL, FALSE);

	// Remove action group
	if (self->vogon_ag)
	{
		gtk_ui_manager_remove_action_group(ui_mng, self->vogon_ag);
		gel_free_and_invalidate(self->vogon_ag, NULL, g_object_unref);
		gtk_ui_manager_ensure_update(ui_mng); 
	}

	// Unmerge UI
	if (self->vogon_merge_id)
	{
		gtk_ui_manager_remove_ui(ui_mng, self->vogon_merge_id);
		gtk_ui_manager_ensure_update(ui_mng);
		self->vogon_merge_id = 0;
	}

	self->status_icon = NULL;

	return TRUE;
}


// -------------
static void
ntfy_reset(EinaNtfy *self)
{
	self->stream  = NULL;
	self->all_tags = FALSE;
	self->got_cover = FALSE;
	gel_free_and_invalidate(self->cover, NULL, g_object_unref);
	if (self->search)
	{
		art_cancel(eina_obj_get_art(self), self->search);
		self->search = NULL;
	}
}

static void
ntfy_sync(EinaNtfy *self)
{
	LomoPlayer *lomo     = eina_obj_get_lomo(self);
	LomoStream *stream   = lomo_player_get_current_stream(lomo);
	gboolean    all_tags = lomo_stream_get_all_tags_flag(stream);

	gboolean update = FALSE;
	if (self->stream != stream)
		update = TRUE;
	if (self->all_tags != all_tags)
		update = TRUE;
	if (self->cover && !self->got_cover)
		update = TRUE;
	
	if (!update)
		return;

	self->stream = stream;
	self->all_tags = all_tags;
	self->got_cover = (self->cover != NULL);

	// Build body
	gchar *tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
	gchar *bname = g_uri_unescape_string(tmp, NULL);
	g_free(tmp);

	gchar *artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	gchar *title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	gchar *body = g_strdup_printf("<b>%s</b>\n%s", title ? title : bname, artist ? artist : "");

	gel_free_and_invalidate(bname, NULL, g_free);

	// Check cover
	if (all_tags)
		self->search = art_search(eina_obj_get_art(self), self->stream, (ArtFunc ) art_search_cb, self);

	GdkPixbuf *cover = self->cover ? self->cover : self->default_cover;
	notify_notification_update(self->ntfy, N_("Playing now"), body, NULL);
	if (cover)
		notify_notification_set_icon_from_pixbuf(self->ntfy, cover);
	notify_notification_show(self->ntfy, NULL);
}
// -------------

static void
app_plugin_init_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self)
{
	if (!g_str_equal(gel_plugin_get_info(plugin)->name, "vogon"))
		return;
	vogon_enable(self);
}

static void
app_plugin_fini_cb(GelApp *app, GelPlugin *plugin, EinaNtfy *self)
{
	if (!g_str_equal(gel_plugin_get_info(plugin)->name, "vogon"))
		return;
	vogon_disable(self);
}


static void
art_search_cb(Art *art, ArtSearch *search, EinaNtfy *self)
{
	self->search = NULL;

	gpointer data = art_search_get_result(search);
	g_return_if_fail(data && GDK_IS_PIXBUF(data));

	self->cover = (GdkPixbuf *) data;
	ntfy_sync(self);
}

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaNtfy *self)
{
	if (stream != self->stream)
		return;
	
	Art *art = eina_obj_get_art(self);
	// Start search
	if (self->search)
		art_cancel(art, self->search);

	self->search = art_search(art, self->stream, (ArtFunc) art_search_cb, self);
	ntfy_sync(self);
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

EINA_PLUGIN_INFO_SPEC(ntfy,
	"0.1.0",
	"lomo,art,settings",

	NULL,
	NULL,

	N_("Notifications plugin"),
	NULL,
	"ntfy.png"
);

