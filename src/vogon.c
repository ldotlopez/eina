#define GEL_DOMAIN "Eina::Vogon"

#include "base.h"
#include "vogon.h"
#include "player.h"
#include "playlist.h"
#include "settings.h"
#include "fs.h"
#include <gmodule.h>
#include <gel/gel-ui.h>

/* * * * * * * * * * */
/* Define ourselves  */
/* * * * * * * * * * */
struct _EinaVogon {
	EinaBase   parent;

	// Add your own fields here
	EinaConf      *conf;
	// EinaFsFilter  *stream_filter;
	GtkStatusIcon *icon;
	GtkWidget     *widget;
	GtkWidget     *menu;
	guint          tooltip_show_timeout_id;

	gint player_x, player_y;
};

gboolean vogon_load_playlist(EinaVogon *self);

/* * * * * * * * * */
/* Lomo Callbacks */
/* * * * * * * * * */
/* Add here callback prototipes for LomoPlayer events: */
/* void on_vogon_lomo_something(LomoPlayer *lomo, ..., EinaVogon *self); */

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
void on_vogon_menu_show_playlist_activate(GtkWidget *w, EinaVogon *self);
void on_vogon_menu_activate(GtkWidget *w, EinaVogon *self);
gboolean on_vogon_destroy(GtkWidget *w, EinaVogon *self);
gboolean on_vogon_activate(GtkWidget *w , EinaVogon *self);
void on_vogon_drag_data_received
    (GtkWidget       *widget,
    GdkDragContext   *drag_context,
    gint              x,
    gint              y,
    GtkSelectionData *data,
    guint             info,
    guint             time,
	EinaVogon *self);

void on_vogon_settings_change(EinaConf *conf, const gchar *key, EinaVogon *self);
void on_vogon_popup_menu(GtkWidget *w, guint button, guint activate_time, EinaVogon *self);

/* * * * * * * * * * * */
/* Signal definitions  */
/* * * * * * * * * * * */
GelUISignalDef _vogon_signals[] = {
	{ "vogon-playlist-window", "delete-event",
		G_CALLBACK(gtk_widget_hide_on_delete) },
	{ "vogon-menu-show-playlist", "activate",
		G_CALLBACK(on_vogon_menu_show_playlist_activate) },
	{ "vogon-menu-play", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-stop", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-pause", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-prev", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-next", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-repeat", "toggled",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-random", "toggled",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-clear-playlist", "activate",
		G_CALLBACK(on_vogon_menu_activate) },
	{ "vogon-menu-quit", "activate",
		G_CALLBACK(on_vogon_menu_activate) },

	GEL_UI_SIGNAL_DEF_NONE
};

void on_g_hub_load(GHub *hub, const gchar *name) {
	gel_info("Got loaded %s", name);
}

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean vogon_init(GHub *hub, gint *argc, gchar ***argv) {
	EinaVogon *self;
	GdkPixbuf *pixbuf  = NULL;
	GError *err = NULL;
	GelUIImageDef img_def = {
		NULL, "vogon-icon.png",
		24, 24
	};

	/*
	 * Create mem in hub (if needed)
	 */
	self = g_new0(EinaVogon, 1);
	if(!eina_base_init((EinaBase *) self, hub, "vogon", EINA_BASE_GTK_UI)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	/* Crete icon */
	self->icon   = gtk_status_icon_new();
	// gtk_ext_load_imagedef_to_pixbuf(img_def, &pixbuf);
	pixbuf = gel_ui_load_pixbuf_from_imagedef(img_def, &err);
	if (pixbuf == NULL) {
		gel_error("Cannot load image '%s': %s", img_def.image, err->message);
		g_error_free(err);
		g_object_unref(self->icon);
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}
	gtk_status_icon_set_from_pixbuf(self->icon, pixbuf);

	/* Create menu */
	g_signal_connect(
		G_OBJECT(self->icon), "popup-menu",
		G_CALLBACK(on_vogon_popup_menu), self);
	g_signal_connect(
		G_OBJECT(self->icon), "activate",
		G_CALLBACK(on_vogon_activate), self);
	gel_ui_signal_connect_from_def_multiple(UI(self), _vogon_signals, self, NULL);

	/* Load settings */
	if (!g_hub_load(hub, "settings")) {
		gel_error("Cannot load settings component");
		g_object_unref(self->icon);
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}
	self->conf = g_hub_shared_get(hub, "settings");
	g_signal_connect(
		self->conf, "change",
		G_CALLBACK(on_vogon_settings_change), self);
	/*
	g_signal_connect(
		G_OBJECT(self->icon), "destroy",
		G_CALLBACK(on_vogon_destroy), self);
	g_signal_connect(
		G_OBJECT(self->widget), "destroy",
		G_CALLBACK(on_vogon_destroy), self);
	g_signal_connect(
		G_OBJECT(self->widget), "button_press_event",
		G_CALLBACK(on_vogon_button_press_event), self);
	g_signal_connect(
		G_OBJECT(self->widget), "drag_data_received",
		G_CALLBACK(on_vogon_drag_data_received), self);
	*/
#if 0
	self->stream_filter = eina_fs_filter_new();
	eina_fs_filter_add_suffix(self->stream_filter, ".mp3");
	eina_fs_filter_add_suffix(self->stream_filter, ".ogg");
	eina_fs_filter_add_suffix(self->stream_filter, ".wav");
	eina_fs_filter_add_suffix(self->stream_filter, ".flac");

	g_idle_add((GSourceFunc) vogon_load_playlist, self);


	// gtk_widget_show_all(GTK_WIDGET(self->icon));
#endif
	return TRUE;
}

G_MODULE_EXPORT gboolean vogon_exit
(gpointer data)
{
	EinaVogon *self = (EinaVogon *) data;
	// eina_fs_filter_free(self->stream_filter);
	g_hub_unload(HUB(self), "settings");
	eina_base_fini((EinaBase *) self);
	return TRUE;
}


void vogon_menu_show(EinaVogon *self) {
	gtk_menu_popup(
		GTK_MENU(W(self, "vogon-menu")),
		NULL,
		NULL,
		NULL,
		NULL,
		3,
		gtk_get_current_event_time()
	);
}

gboolean vogon_load_playlist(EinaVogon *self) {
	EinaPlaylist  *pl;
	GtkWidget     *pl_widget;

	/* Load playlist */

	if (!g_hub_load(HUB(self), "playlist")) 
		goto fail;

	if ((pl = g_hub_shared_get(HUB(self), "playlist")) == NULL)
		goto fail;

	if ((pl_widget = playlist_get_widget(pl)) == NULL)
		goto fail;

	gtk_container_add(GTK_CONTAINER(W(self, "vogon-playlist-window")), pl_widget);
	gtk_widget_hide_on_delete(W(self, "vogon-playlist-window"));
	
	return FALSE;

fail:
	gel_warn("Cannot load playlist, disabling 'Show playlist item'");
	gtk_widget_hide(W(self, "vogon-menu-show-playlist"));
	gtk_widget_hide(W(self, "vogon-menu-separator-1"));
	return FALSE;
}

void vogon_show_playlist(EinaVogon *self) {
	gtk_widget_show(W(self, "vogon-playlist-window"));
}

/* * * * * * * * * * * * * */
/* Implement UI Callbacks  */
/* * * * * * * * * * * * * */
void on_vogon_menu_show_playlist_activate(GtkWidget *w, EinaVogon *self) {
	vogon_show_playlist(self);
}


void on_vogon_popup_menu(GtkWidget *w, guint button, guint activate_time, EinaVogon *self) {
    gtk_menu_popup(
        GTK_MENU(W(self, "vogon-menu")),
        NULL,
        NULL,
        NULL,
        NULL,
        button,
        activate_time
    );
}

void on_vogon_menu_activate(GtkWidget *w, EinaVogon *self) {
	GError *error = NULL;
	
	if ( w == W(self, "vogon-menu-play")) {
		lomo_player_play(LOMO(self), &error);
	}

	else if ( w == W(self, "vogon-menu-pause")) {
		lomo_player_pause(LOMO(self), &error);
	}
	
	else if ( w == W(self, "vogon-menu-stop")) {
		lomo_player_stop(LOMO(self), &error);
	}

	else if ( w == W(self, "vogon-menu-prev")) {
		lomo_player_go_prev(LOMO(self));
	}

	else if ( w == W(self, "vogon-menu-next")) {
		lomo_player_go_next(LOMO(self));
	}

	else if ( w == W(self, "vogon-menu-repeat")) {
		lomo_player_set_repeat(
			LOMO(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)));
		eina_conf_set_bool(
			self->conf, "/core/repeat",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)));
	}
	
	else if ( w == W(self, "vogon-menu-random")) {
		lomo_player_set_random(
			LOMO(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)));
		eina_conf_set_bool(
			self->conf, "/core/random",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)));
	}

	else if ( w == W(self, "vogon-menu-clear-playlist")) {
		lomo_player_clear(LOMO(self));
	}

	
	else if ( w == W(self, "vogon-menu-quit")) {
		g_object_unref(HUB(self));
		// vogon_exit(self);
	}

	else {
		gel_warn("Unknow item");
	}

	if (error != NULL) {
		gel_error("%s", error->message);
		g_error_free(error);
	}
}

gboolean on_vogon_destroy(GtkWidget *w, EinaVogon *self) {
	GtkWidget *window = W(self, "main-window");

	if ( !GTK_WIDGET_VISIBLE(window) )
		gtk_widget_show(window);
	return FALSE;
}

gboolean on_vogon_activate
(GtkWidget *w, EinaVogon *self)
{
	EinaPlayer *player;
	GtkWidget  *window;
	if (!g_hub_loaded(HUB(self), "player"))
		return FALSE;

	player = g_hub_shared_get(HUB(self), "player");
	window = W(player, "main-window");

	if ( GTK_WIDGET_VISIBLE(window) ) {
		gtk_window_get_position(GTK_WINDOW(window),
			&self->player_x,
			&self->player_y);
		gtk_widget_hide(window);
	}
	else {
		gtk_window_move(GTK_WINDOW(window),
			self->player_x,
			self->player_y);
		gtk_widget_show(window);
	}
	return FALSE;
}

void on_vogon_drag_data_received
    (GtkWidget       *widget,
    GdkDragContext  *drag_context,
    gint             x,
    gint             y,
    GtkSelectionData *data,
    guint            info,
    guint            time,
    EinaVogon     *self) {
#if 0
	gint i;
	gchar *filename;
	gchar **uris;
	GSList *uri_list = NULL, *uri_scan, *uri_filter;

	uris = g_uri_list_extract_uris((gchar *) data->data);
    for ( i = 0; uris[i] != NULL; i++ ) {
        filename = g_filename_from_uri(uris[i], NULL, NULL);
        uri_list = g_slist_prepend(uri_list, g_strconcat("file://", filename, NULL));
        g_free(filename);
    }
    g_strfreev(uris);
    uri_list = g_slist_reverse(uri_list);

    uri_scan = eina_fs_scan(uri_list); //  Scan
    g_ext_slist_free(uri_list, g_free);

    uri_filter = eina_fs_filter_filter(self->stream_filter, // Filter
        uri_scan,
        TRUE, FALSE, FALSE,
        TRUE, FALSE);

    lomo_player_clear(LOMO(self));
    lomo_player_add_uri_multi(LOMO(self), (GList *) uri_filter);

    g_slist_free(uri_filter);
    g_ext_slist_free(uri_scan, g_free);
#endif
}

void on_vogon_settings_change(EinaConf *conf, const gchar *key, EinaVogon *self) {
	if (g_str_equal(key, "/core/repeat")) {
		gtk_toggle_action_set_active (
			GTK_TOGGLE_ACTION(W(self, "vogon-menu-repeat")),
			eina_conf_get_bool(conf, "/core/repeat", TRUE));
	}
	
	else if (g_str_equal(key, "/core/random")) {
		gtk_toggle_action_set_active (
			GTK_TOGGLE_ACTION(W(self, "vogon-menu-random")),
			eina_conf_get_bool(conf, "/core/random", TRUE));
	}
}

/* * * * * * * * * * ** * * */
/* Implement Lomo Callbacks */
/* * * * * * * * * * ** * * */

/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GHubSlave vogon_connector = {
	"vogon",
	&vogon_init,
	&vogon_exit
};

