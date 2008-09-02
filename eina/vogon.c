#define GEL_DOMAIN "Eina::Vogon"

#include <string.h>
#include "base.h"
#include "vogon.h"
#include "player.h"
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
	GtkStatusIcon *icon;
	GtkWidget     *widget;
	GtkWidget     *menu;
	guint          tooltip_show_timeout_id;

	GtkUIManager  *ui_mng;

	gint player_x, player_y;
};

/* * * * * * * * * */
/* Lomo Callbacks */
/* * * * * * * * * */
/* Add here callback prototipes for LomoPlayer events: */
/* void on_vogon_lomo_something(LomoPlayer *lomo, ..., EinaVogon *self); */

/* * * * * * * * */
/* UI Callbacks  */
/* * * * * * * * */
void on_vogon_menu_activate(GtkAction *action, EinaVogon *self);
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
void on_vogon_menu_popup(GtkWidget *w, guint button, guint activate_time, EinaVogon *self);

/*
 * Init/Exit functions 
 */

static const gchar ui_def[] =
"<ui>"
"	<popup name='MainMenu'>"
"		<menuitem action='Play'  />"
"		<menuitem action='Pause' />"
"		<menuitem action='Stop'  />"
"		<separator />"
"		<menuitem action='Next' />"
"		<menuitem action='Previous'  />"
"		<separator />"
"		<menuitem action='Repeat' />"
"		<menuitem action='Shuffle' />"
"		<separator />"
"		<menuitem action='Clear' />"
"		<menuitem action='Quit' />"
"	</popup>"
"</ui>";

G_MODULE_EXPORT gboolean vogon_init(GelHub *hub, gint *argc, gchar ***argv) {
	EinaVogon *self;
	GdkPixbuf *pixbuf  = NULL;
	GError *err = NULL;
	GelUIImageDef img_def = {
		NULL, "vogon-icon.png",
		24, 24
	};

	GtkActionGroup *ag;

	const GtkActionEntry ui_actions[] = {
		/* Menu actions */
		{ "Play", GTK_STOCK_MEDIA_PLAY, N_("Play"),
			"<alt>x", "Play", G_CALLBACK(on_vogon_menu_activate) },
		{ "Pause", GTK_STOCK_MEDIA_PAUSE, N_("Pause"),
			"<alt>c", "Pause", G_CALLBACK(on_vogon_menu_activate) },
		{ "Stop", GTK_STOCK_MEDIA_STOP, N_("Stop"),
			"<alt>v", "Stop", G_CALLBACK(on_vogon_menu_activate) },
		{ "Next", GTK_STOCK_MEDIA_NEXT, N_("Next"),
			"<alt>b", "Next stream", G_CALLBACK(on_vogon_menu_activate) },
		{ "Previous", GTK_STOCK_MEDIA_PREVIOUS, N_("Previous"),
			"<alt>z", "Previous stream", G_CALLBACK(on_vogon_menu_activate) },
		{ "Clear", GTK_STOCK_CLEAR, N_("C_lear"),
			"<alt>l", "Clear playlist", G_CALLBACK(on_vogon_menu_activate) },
		{ "Quit", GTK_STOCK_QUIT, N_("_Quit"),
			"<alt>q", "Quit Eina", G_CALLBACK(on_vogon_menu_activate) }
	};
	const GtkToggleActionEntry ui_toggle_actions[] = {
		{ "Shuffle", NULL, N_("Shuffle"),
			"<alt>s", "Shuffle playlist", G_CALLBACK(on_vogon_menu_activate) },
		{ "Repeat", NULL , N_("Repeat"),
			"<alt>r", "Repeat playlist", G_CALLBACK(on_vogon_menu_activate) }
	};

	/*
	 * Create mem in hub (if needed)
	 */
	self = g_new0(EinaVogon, 1);
	if(!eina_base_init((EinaBase *) self, hub, "vogon", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	/* Crete icon */
	self->icon   = gtk_status_icon_new();
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
	self->ui_mng = gtk_ui_manager_new();
	gtk_ui_manager_add_ui_from_string(self->ui_mng, ui_def, -1, NULL);

	ag = gtk_action_group_new("default");
	gtk_action_group_add_actions(ag, ui_actions, G_N_ELEMENTS(ui_actions), self);
	gtk_action_group_add_toggle_actions(ag, ui_toggle_actions, G_N_ELEMENTS(ui_toggle_actions), self);

	gtk_ui_manager_insert_action_group(self->ui_mng, ag, 0);
	gtk_ui_manager_ensure_update(self->ui_mng);

	self->menu = gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu");

	g_signal_connect(self->icon, "popup-menu",
		G_CALLBACK(on_vogon_menu_popup), self);

	/* Load settings */
	if (!gel_hub_load(hub, "settings")) {
		gel_error("Cannot load settings component");
		g_object_unref(self->ui_mng);
		g_object_unref(self->icon);
		eina_base_fini(EINA_BASE(self));
		return FALSE;
	}

	self->conf = gel_hub_shared_get(hub, "settings");

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat")),
		eina_conf_get_bool(self->conf, "/core/repeat", FALSE));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle")),
		eina_conf_get_bool(self->conf, "/core/random", FALSE));

	g_signal_connect(
		self->conf, "change",
		G_CALLBACK(on_vogon_settings_change), self);
	g_signal_connect(
		G_OBJECT(self->icon), "activate",
		G_CALLBACK(on_vogon_activate), self);

	return TRUE;
}

G_MODULE_EXPORT gboolean vogon_exit
(gpointer data)
{
	EinaVogon *self = (EinaVogon *) data;

	g_object_unref(self->icon);
	g_object_unref(self->ui_mng);

	gel_hub_unload(HUB(self), "settings");

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

/* * * * * * * * * * * * * */
/* Implement UI Callbacks  */
/* * * * * * * * * * * * * */
void on_vogon_menu_popup(GtkWidget *w, guint button, guint activate_time, EinaVogon *self) {
    gtk_menu_popup(
        GTK_MENU(self->menu),
        NULL, NULL, NULL, NULL,
        button,
        activate_time
    );
}

void on_vogon_menu_activate(GtkAction *action, EinaVogon *self) {
	GError *error = NULL;
	const gchar *name = gtk_action_get_name(action);

	if (g_str_equal(name, "Play")) {
		lomo_player_play(LOMO(self), &error);
	}

	else if (g_str_equal(name, "Pause")) {
		lomo_player_pause(LOMO(self), &error);
	}
	
	else if (g_str_equal(name, "Stop")) {
		lomo_player_stop(LOMO(self), &error);
	}

	else if (g_str_equal(name, "Previous")) {
		lomo_player_go_prev(LOMO(self));
	}

	else if (g_str_equal(name, "Next")) {
		lomo_player_go_next(LOMO(self));
	}

	else if (g_str_equal(name, "Repeat")) {
		lomo_player_set_repeat(
			LOMO(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
		eina_conf_set_bool(
			self->conf, "/core/repeat",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
	}
	
	else if (g_str_equal(name, "Shuffle")) {
		lomo_player_set_random(
			LOMO(self),
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
		eina_conf_set_bool(
			self->conf, "/core/random",
			gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
	}

	else if (g_str_equal(name, "Clear")) {
		lomo_player_clear(LOMO(self));
	}

	
	else if (g_str_equal(name, "Quit")) {
		g_object_unref(HUB(self));
	}

	else {
		gel_warn("Unknow item: %s", name);
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
	if (!gel_hub_loaded(HUB(self), "player"))
		return FALSE;

	player = gel_hub_shared_get(HUB(self), "player");
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
	    gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Repeat")),
			eina_conf_get_bool(conf, "/core/repeat", TRUE));
	}
	
	else if (g_str_equal(key, "/core/random")) {
	    gtk_check_menu_item_set_active(
			GTK_CHECK_MENU_ITEM(gtk_ui_manager_get_widget(self->ui_mng, "/MainMenu/Shuffle")),
			eina_conf_get_bool(conf, "/core/random", TRUE));
	}
}

/* * * * * * * * * * * * * * * * * * */
/* Create the connector for the hub  */
/* * * * * * * * * * * * * * * * * * */
G_MODULE_EXPORT GelHubSlave vogon_connector = {
	"vogon",
	&vogon_init,
	&vogon_exit
};

