/*
 * eina/player.c
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

#define GEL_DOMAIN "Eina::Player"
#define EINA_PLUGIN_DATA_TYPE EinaPlayer

#include <gel/gel-io.h>

// Modules
#include <eina/eina-plugin.h>
#include <eina/ext/eina-seek.h>
#include <eina/ext/eina-volume.h>
#include <eina/about.h>
#include <eina/player.h>

#define HELP_URI "http://answers.launchpad.net/eina"
#define BUGS_URI "https://bugs.launchpad.net/eina/+filebug"

struct _EinaPlayer {
	EinaObj parent;
	GtkActionGroup *action_group;
	guint merge_id;

	GQueue *io_tree_files;
	GelIOTreeOp *io_op;
};

static void
player_update_state(EinaPlayer *self);
static void
player_update_information(EinaPlayer *self);
static void
player_dnd_setup(EinaPlayer *self);

static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream);

static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self);
static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self);
static void
volume_value_changed_cb(GtkScaleButton *w, gdouble value, EinaPlayer *self);
static void
action_activated_cb(GtkAction *action, EinaPlayer *self);

static void
io_tree_read_success_cb(GelIOTreeOp *op, const GFile *source, const GNode *result, EinaPlayer *self);
static void
io_tree_read_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error, gpointer data);

static gchar *ui_xml = 
"<ui>"
"<menubar name='Main'>"
"  <menu name='File' action='FileMenu'>"
"    <menuitem name='Open' action='open-action' />"
"    <menuitem name='Quit' action='quit-action' />"
"  </menu>"
"  <menu name='Help' action='HelpMenu'>"
"    <menuitem name='Help'  action='help-action'  />"
"    <menuitem name='Bug'   action='bug-action'   />"
"    <menuitem name='About' action='about-action' />"
"  </menu>"
"</menubar>"
"</ui>"
;

static gboolean
player_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = NULL;

	// Initialize base class
	self = g_new0(EINA_PLUGIN_DATA_TYPE, 1);
	if (!eina_obj_init((EinaObj *) self, plugin, "player", EINA_OBJ_NONE, error))
	{
		g_free(self);
		return FALSE;
	}

	gchar *objs[] = {"main-widget",
		"prev-action",
		"play-action",
		"next-action",
		"open-action",
		"pause-action",
		"quit-action",
		"help-action",
		"about-action",
		"bug-action",
		NULL };
	if (!eina_obj_load_objects_from_resource((EinaObj *) self, "player.ui", objs, error))
	{
		eina_obj_fini((EinaObj *) self);
		return FALSE;
	}

	self->io_tree_files = g_queue_new();
	plugin->data = self;

	//
	// Setup UI bits
	//

	// Seek widget
    EinaSeek *seek = eina_seek_new();
    g_object_set(seek,
	    "lomo-player",     eina_obj_get_lomo(self),
	    "current-label",   eina_obj_get_typed(self, GTK_LABEL, "time-current-label"),
	    "remaining-label", eina_obj_get_typed(self, GTK_LABEL, "time-remaining-label"),
	    "total-label",     eina_obj_get_typed(self, GTK_LABEL, "time-total-label"),
	    NULL);

    gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "seek-container"),
		GTK_WIDGET(seek));
		gtk_widget_show(GTK_WIDGET(seek));

	// Volume widget
	EinaVolume *volume = eina_volume_new();
	eina_volume_set_lomo_player(volume, eina_obj_get_lomo(self));
	gel_ui_container_replace_children(
		eina_obj_get_typed(self, GTK_CONTAINER, "volume-button-container"),
		GTK_WIDGET(volume));
		gtk_widget_show(GTK_WIDGET(volume));

	// Enable actions
	gint i;
	self->action_group = gtk_action_group_new("player");
	const gchar *actions[] = {
		"play-action", "pause-action",
		"next-action", "prev-action",
		"open-action", "quit-action",
		"help-action", "bug-action", "about-action" };
	const gchar *accels[] = {
		"", "",
		"Page_Down", "Page_Up",
		"<Control>o", "<Control>q",
		"F1", "", "" };
	for (i = 0; i < G_N_ELEMENTS(actions); i++)
	{
		GtkAction *action = eina_obj_get_typed(self, GTK_ACTION, actions[i]);
		gtk_action_group_add_action_with_accel(self->action_group, action, accels[i]);
		g_signal_connect((GObject *) action, "activate", (GCallback) action_activated_cb, self);
	
	}
	GtkUIManager *ui_mng = eina_window_get_ui_manager(eina_obj_get_window(self));
	gtk_ui_manager_insert_action_group(ui_mng, self->action_group, G_MAXINT);

	// UI Manager
	GError *err = NULL;
	if ((self->merge_id = gtk_ui_manager_add_ui_from_string(ui_mng, ui_xml, -1, &err)) == 0)
	{
		gel_warn(N_("Cannot merge with UI Manager: %s"), err->message);
		g_error_free(err);
		return FALSE;
	}
	else
		gtk_ui_manager_ensure_update(ui_mng);

	// play/pause state
	player_update_state(self);

	// Information
	player_update_information(self);

	// Connect lomo signals
	g_signal_connect(volume, "value-changed", (GCallback) volume_value_changed_cb, self);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_signal_connect_swapped(lomo, "play",   (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "pause",  (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "stop",   (GCallback) player_update_state, self);
	g_signal_connect_swapped(lomo, "change", (GCallback) player_update_information, self);
	g_signal_connect(lomo, "clear",    (GCallback) lomo_clear_cb, self);
	g_signal_connect(lomo, "all-tags", (GCallback) lomo_all_tags_cb, self);

	player_dnd_setup(self);

	// Add to main window
	GtkWidget *main_widget = eina_obj_get_typed(self, GTK_WIDGET, "main-widget");
	eina_window_add_widget(eina_obj_get_window(self), main_widget, FALSE, TRUE, 0);
	gtk_widget_show(main_widget);

	return TRUE;
}

static gboolean
player_fini(GelApp *app, GelPlugin *plugin, GError **error)
{
	EinaPlayer *self = EINA_PLUGIN_DATA(plugin);

	gel_free_and_invalidate(self->io_op, NULL, gel_io_tree_op_close);
	g_queue_foreach(self->io_tree_files, (GFunc) g_object_unref, NULL);
	g_queue_clear(self->io_tree_files);

	eina_window_remove_widget(eina_obj_get_window(self), eina_obj_get_typed(self, GTK_WIDGET, "main-widget"));
	eina_obj_fini(EINA_OBJ(plugin->data));

	return TRUE;
}

GtkContainer *
eina_player_get_cover_container(EinaPlayer* self)
{
	return eina_obj_get_typed(self, GTK_CONTAINER, "cover-container");
}

// --
// Internal API
// --
static void
player_update_state(EinaPlayer *self)
{
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_return_if_fail(lomo != NULL);

	GtkActivatable *button = eina_obj_get_typed(self, GTK_ACTIVATABLE, "play-pause-button");
	GtkImage       *image  = eina_obj_get_typed(self, GTK_IMAGE,       "play-pause-image");

	const gchar *action = NULL;
	const gchar *stock = NULL;

	if (lomo_player_get_state(lomo) == LOMO_STATE_PLAY)
	{
		action = "pause-action";
		stock  = "gtk-media-pause";
	}
	else
	{
		action = "play-action";
		stock  = "gtk-media-play";
	}
	gtk_activatable_set_related_action(button, eina_obj_get_typed(self, GTK_ACTION, action));
	gtk_image_set_from_stock(image, stock, GTK_ICON_SIZE_BUTTON);
}

static void
player_update_information(EinaPlayer *self)
{
	gchar *info   = "<span size=\"x-large\" weight=\"bold\">Eina music player</span>\n<span size=\"x-large\" weight=\"normal\">\u200B</span>";
	gchar *markup = "<span size=\"x-large\" weight=\"bold\">%t</span>\n<span size=\"x-large\" weight=\"normal\">{%a}</span>";

	GtkWidget *label = eina_obj_get_typed(self, GTK_WIDGET, "stream-info-label");

	LomoPlayer *lomo;
	LomoStream *stream;

	if (!(lomo = eina_obj_get_lomo(self)) || !(stream = lomo_player_get_current_stream(lomo)))
	{
		g_object_set(label,
			"selectable", FALSE,
			"use-markup", TRUE,
			"label", info,
			NULL);
		gtk_window_set_title((GtkWindow *) eina_obj_get_window(self), "Eina player");
		return;
	}

	info = gel_str_parser(markup, (GelStrParserFunc) stream_info_parser_cb, stream);
	g_object_set(label,
		"selectable", TRUE,
		"use-markup", TRUE,
		"label", info,
		NULL);
	g_free(info);

	gchar *title = g_strdup(lomo_stream_get_tag(stream, LOMO_TAG_TITLE));
	if (title == NULL)
	{
		gchar *tmp = g_path_get_basename(lomo_stream_get_tag(stream, LOMO_TAG_URI));
		title =  g_uri_unescape_string(tmp, NULL);
		g_free(tmp);
   }

   gtk_window_set_title((GtkWindow *) eina_obj_get_window(self), title);
   g_free(title);
}

static gchar *
stream_info_parser_cb(gchar key, LomoStream *stream)
{
	gchar *ret = NULL;
	gchar *tag_str = lomo_stream_get_tag_by_id(stream, key);

	if (tag_str != NULL)
	{
		ret = g_markup_escape_text(tag_str, -1);
		g_free(tag_str);
	}

	if ((key == 't') && (ret == NULL))
	{
		const gchar *tmp = lomo_stream_get_tag(stream, LOMO_TAG_URI);
		gchar *tmp2 = g_uri_unescape_string(tmp, NULL);
		ret = g_path_get_basename(tmp2);
		g_free(tmp2);
	}
	return ret;
}

// --
// Signals
// --
static void
lomo_all_tags_cb(LomoPlayer *lomo, LomoStream *stream, EinaPlayer *self)
{
	if (stream == lomo_player_get_current_stream(lomo))
		player_update_information(self);
}

static void
lomo_clear_cb(LomoPlayer *lomo, EinaPlayer *self)
{
	gel_free_and_invalidate(self->io_op, NULL, gel_io_tree_op_close);
	g_queue_foreach(self->io_tree_files, (GFunc) g_object_unref, NULL);
	g_queue_clear(self->io_tree_files);

	player_update_information(self);
}

static void
volume_value_changed_cb(GtkScaleButton *w, gdouble value, EinaPlayer *self)
{
	lomo_player_set_volume(eina_obj_get_lomo(self), (gint) (value * 100));
}

static void
action_activated_cb(GtkAction *action, EinaPlayer *self)
{
	const gchar *name = gtk_action_get_name(action);

	LomoPlayer *lomo = eina_obj_get_lomo(self);
	g_return_if_fail(lomo != NULL);

	GError *error = NULL;
	if (g_str_equal(name, "play-action"))
		lomo_player_play(lomo, &error);

	else if (g_str_equal(name, "pause-action"))
		lomo_player_pause(lomo, &error);

	else if (g_str_equal(name, "next-action"))
		lomo_player_go_next(lomo, &error);

	else if (g_str_equal(name, "prev-action"))
		lomo_player_go_prev(lomo, &error);

	else if (g_str_equal(name, "volume-action"))
	{
	}
	
	else if (g_str_equal(name, "open-action"))
		eina_fs_file_chooser_load_files(lomo);

	else if (g_str_equal(name, "quit-action"))
		g_object_unref(eina_obj_get_app(self));

	else if (g_str_equal(name, "help-action"))
		gtk_show_uri(NULL, HELP_URI, GDK_CURRENT_TIME, &error);

	else if (g_str_equal(name, "bug-action"))
		gtk_show_uri(NULL, BUGS_URI, GDK_CURRENT_TIME, &error);

	else if (g_str_equal(name, "about-action"))
	{
		eina_about_show(eina_obj_get_about((EinaObj *) self));
	}

	else
	{
		gel_warn(N_("Unknow action: %s"), name);
		return;
	}

	if (error != NULL)
	{
		gel_error(N_("Unable to complete action '%s': %s"), name, error->message);
		g_error_free(error);
	}
}

// ---
// DnD
// ---
enum {
	DND_TARGET_STRING
};

static GtkTargetEntry dnd_target_list[] = {
	{ "STRING",     0, DND_TARGET_STRING },
	{ "text/plain", 0, DND_TARGET_STRING }
};

static guint dnd_n_targets = G_N_ELEMENTS(dnd_target_list);

/******************************************************************************/
/* Signal receivable by destination */

/* Emitted when the data has been received from the source. It should check
 * the GtkSelectionData sent by the source, and do something with it. Finally
 * it needs to finish the operation by calling gtk_drag_finish, which will emit
 * the "data-delete" signal if told to. */
static void
drag_data_received_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
        GtkSelectionData *selection_data, guint target_type, guint time,
	EinaPlayer *self)
{
	gchar   *_sdata;

	gboolean dnd_success = FALSE;
	gboolean delete_selection_data = FALSE;

	/* Deal with what we are given from source */
	if((selection_data != NULL) && (selection_data-> length >= 0))
	{
		if (context-> action == GDK_ACTION_ASK)
		{
			/* Ask the user to move or copy, then set the context action. */
		}

		if (context-> action == GDK_ACTION_MOVE)
			delete_selection_data = TRUE;

		/* Check that we got the format we can use */
		switch (target_type)
		{
			case DND_TARGET_STRING:
				_sdata = (gchar*) selection_data-> data;
				dnd_success = TRUE;
				break;

			default:
				gel_warn("Unknow DnD type");
		}

	}

	if (dnd_success == FALSE)
	{
		gel_error("DnD data transfer failed!\n");
	}
	else
	{
		lomo_player_clear(eina_obj_get_lomo(self));
		gchar **uris = g_uri_list_extract_uris(_sdata);
		gint i;
		for (i = 0; uris[i] && uris[i][0] ; i++)
			g_queue_push_tail(self->io_tree_files, g_file_new_for_uri(uris[i]));
		
		g_strfreev(uris);
	}

	gtk_drag_finish (context, dnd_success, delete_selection_data, time);

	if (g_queue_is_empty(self->io_tree_files))
		return;

	if (self->io_op)
	{
		gel_io_tree_op_close(self->io_op);
		self->io_op = NULL;
	}
	
	self->io_op = gel_io_tree_walk(g_queue_pop_head(self->io_tree_files), "standard::*", TRUE,
		(GelIOTreeSuccessFunc) io_tree_read_success_cb, io_tree_read_error_cb, self);
}

/* Emitted when a drag is over the destination */
static gboolean
drag_motion_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t,
	gpointer user_data)
{
	// Fancy stuff here. This signal spams the console something horrible.
	//const gchar *name = gtk_widget_get_name (widget);
	//g_print ("%s: drag_motion_cb\n", name);
	return  FALSE;
}

/* Emitted when a drag leaves the destination */
static void
drag_leave_cb
(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data)
{
}

/* Emitted when the user releases (drops) the selection. It should check that
 * the drop is over a valid part of the widget (if its a complex widget), and
 * itself to return true if the operation should continue. Next choose the
 * target type it wishes to ask the source for. Finally call gtk_drag_get_data
 * which will emit "drag-data-get" on the source. */
static gboolean
drag_drop_cb
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time,
	gpointer user_data)
{
	gboolean        is_valid_drop_site;
	GdkAtom         target_type;

	/* Check to see if (x,y) is a valid drop site within widget */
	is_valid_drop_site = TRUE;

	/* If the source offers a target */
	if (context-> targets)
	{
		/* Choose the best target type */
		target_type = GDK_POINTER_TO_ATOM(g_list_nth_data (context->targets, DND_TARGET_STRING));

		/* Request the data from the source. */
		gtk_drag_get_data
		(
			widget,         /* will receive 'drag-data-received' signal */
			context,        /* represents the current state of the DnD */
			target_type,    /* the target type we want */
			time            /* time stamp */
		);
	}

	/* No target offered by source => error */
	else
	{
		is_valid_drop_site = FALSE;
	}

	return  is_valid_drop_site;
}

static void
player_dnd_setup(EinaPlayer *self)
{
	GtkWidget *dest = eina_obj_get_widget((EinaObj*) self, "main-widget");
	gtk_drag_dest_set
	(
		dest,                        /* widget that will accept a drop */
		GTK_DEST_DEFAULT_DROP        /* default actions for dest on DnD */
		| GTK_DEST_DEFAULT_MOTION 
		| GTK_DEST_DEFAULT_HIGHLIGHT,
		dnd_target_list,             /* lists of target to support */
		dnd_n_targets,               /* size of list */
		GDK_ACTION_COPY              /* what to do with data after dropped */
	);

	/* All possible destination signals */
	g_signal_connect (dest, "drag-data-received", G_CALLBACK(drag_data_received_cb), self);
	g_signal_connect (dest, "drag-leave",         G_CALLBACK (drag_leave_cb),        self);
	g_signal_connect (dest, "drag-motion",        G_CALLBACK (drag_motion_cb),       self);
	g_signal_connect (dest, "drag-drop",          G_CALLBACK (drag_drop_cb),         self);
}

static void
io_tree_read_success_cb(GelIOTreeOp *op, const GFile *source, const GNode *result, EinaPlayer *self)
{
	GList *l = gel_io_tree_result_flatten(result);
	GList *i = l;
	while (i)
	{
		GFile     *file = G_FILE(i->data);
		GFileInfo *info = g_object_get_data((GObject *) i->data, "gfileinfo");
		if ((eina_fs_is_supported_file(file)) && (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR))
		{
			gchar *uri = g_file_get_uri((GFile *) i->data);
			lomo_player_append_uri(eina_obj_get_lomo(self), uri);
			g_free(uri);
		}
		i = i->next;
	}

	gel_io_tree_op_close(op);
	g_list_free(l);

	if (g_queue_is_empty(self->io_tree_files))
	{
		self->io_op = NULL;
		return;
	}

	self->io_op = gel_io_tree_walk(g_queue_pop_head(self->io_tree_files), "standard::*", TRUE,
		(GelIOTreeSuccessFunc) io_tree_read_success_cb, io_tree_read_error_cb, self);
}

static void
io_tree_read_error_cb(GelIOTreeOp *op, const GFile *source, const GError *error, gpointer data)
{
	gchar *uri = g_file_get_uri((GFile *) source);
	gel_warn(N_("Error on file %s: %s"), uri, error->message);
	g_free(uri);
}

// --
// Connector 
// --
EINA_PLUGIN_SPEC (player,
	PACKAGE_VERSION,
	"about,lomo,window",
	NULL,
	NULL,

	N_("Build-in player plugin"),
	NULL,
	NULL,

	player_init, player_fini
);

