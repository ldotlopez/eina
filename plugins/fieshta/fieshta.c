/*
 * plugins/fieshta/fieshta.c
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

#define GEL_DOMAIN "Eina::Plugin::Fieshta"
#define GEL_PLUGIN_DATA_TYPE EinaFieshta
#include <eina/eina-plugin.h>
#include <plugins/fieshta/fieshta.h>
#include "fieshta-stage.h"
#include "fieshta-stream.h"
#include <clutter-gtk/gtk-clutter-embed.h>

#define SLOTS 5

// Block tipes
enum {
	BLOCK_SEEK_FF,
	BLOCK_SEEK_REW,
	BLOCK_DELETE,
	N_BLOCKS
};

struct _EinaFieshta {
	EinaObj parent;
	guint ui_merge;
	GtkActionGroup *ag;

	gboolean repeat, random;
	gboolean loop_done;
	gboolean options[N_BLOCKS];

	GtkWidget    *embed;
	FieshtaStage *stage;
};

static gchar *fieshta_ui_xml = 
	"<ui>"
	"  <menubar name=\"MainMenuBar\">"
	"    <menu name=\"FieshtaMenu\" action=\"fiesta-menu-action\">"
	"      <menuitem name=\"FieshtaMode\" action=\"fieshta-mode-toogle-action\"/>"
	"    </menu>"
	"</menubar>"
	"</ui>";

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaFieshta *self);
static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaFieshta *self);
static gboolean
fieshta_hook(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaFieshta *self);
static void
set_fieshta_mode_activated_cb(GtkAction *action, EinaFieshta *self);

static void
fieshta_ui_init(EinaFieshta *self);

gboolean
fieshta_init(GelApp *app, GelPlugin *plugin, GError **error)
{
	gel_warn("Fieshta started");

	EinaFieshta *self = g_new0(EinaFieshta, 1);
	if (!eina_obj_init((EinaObj*) self, plugin, "fieshta", EINA_OBJ_NONE, error))
		return FALSE;
	plugin->data = self;

	EinaWindow   *window     = GEL_APP_GET_WINDOW(app);
	GtkUIManager *ui_manager = eina_window_get_ui_manager(window);
	if ((self->ui_merge = gtk_ui_manager_add_ui_from_string(ui_manager, fieshta_ui_xml, -1, error)) == 0)
		return FALSE;

	GtkActionEntry actions[] = {
		{ "fiesta-menu-action", NULL, N_("Fieshta"),
		NULL, NULL, NULL },
	};
	GtkToggleActionEntry toggle_actions[] = {
		{ "fieshta-mode-toogle-action", NULL, N_("Set fieshta mode"),
		NULL, NULL, G_CALLBACK(set_fieshta_mode_activated_cb) }
	};
	self->ag = gtk_action_group_new("fieshta");
	gtk_action_group_add_actions(self->ag, actions, G_N_ELEMENTS(actions), self);
	gtk_action_group_add_toggle_actions(self->ag, toggle_actions, G_N_ELEMENTS(actions), self);
	gtk_ui_manager_insert_action_group(ui_manager, self->ag, 0);
	gtk_ui_manager_ensure_update(ui_manager);

	self->options[BLOCK_SEEK_FF]  = TRUE;
	self->options[BLOCK_SEEK_REW] = TRUE;
	self->options[BLOCK_DELETE]   = TRUE;

	return TRUE;
}

gboolean
fieshta_fini(GelApp *app, GelPlugin *self, GError **error)
{
	gel_warn("Fieshta ended");
	return TRUE;
}

void
eina_fieshta_set_mode(EinaFieshta *self, gboolean mode)
{
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	if (mode)
	{
		// Enable fieshta
		self->repeat = lomo_player_get_repeat(lomo);
		self->random = lomo_player_get_random(lomo);
		lomo_player_stop(lomo, NULL);
		lomo_player_go_nth(lomo, 0, NULL);
		lomo_player_set_random(lomo, FALSE);
		lomo_player_hook_add(lomo, (LomoPlayerHook) fieshta_hook, self);
		g_signal_connect(lomo, "change", (GCallback) lomo_change_cb, self);
		g_signal_connect(lomo, "insert",    (GCallback) lomo_insert_cb,    self);

		// Build GTK
		fieshta_ui_init(self);
		/*
		gtk_clutter_init(NULL, NULL);

		ClutterColor black = {0, 0, 0, 0};
	
		self->embed = gtk_clutter_embed_new();
		ClutterActor *s = gtk_clutter_embed_get_stage((GtkClutterEmbed *) self->embed);
		clutter_actor_set_size(s, 1280, 768);
		clutter_stage_set_color((ClutterStage *) s, &black);

		self->stage = fieshta_stage_new();
		clutter_actor_set_size((ClutterActor *) self->stage,  1280, 768);
		fieshta_stage_set_slots(self->stage, 5);

		clutter_container_add_actor((ClutterContainer *) s, (ClutterActor *)self->stage);
		clutter_actor_show_all((ClutterActor *) s);

		GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_container_add((GtkContainer *) w, (GtkWidget *) self->embed);
		gtk_widget_show_all(w);
		*/
	}
	else
	{
		lomo_player_set_repeat(lomo, self->repeat);
		lomo_player_set_random(lomo, self->random);
		lomo_player_hook_remove(lomo, (LomoPlayerHook) fieshta_hook);
		g_signal_handlers_disconnect_by_func(lomo, (GCallback ) lomo_change_cb, self);
		g_signal_handlers_disconnect_by_func(lomo, (GCallback ) lomo_insert_cb,    self);

		g_object_unref(self->stage);
	}
}

static void
fieshta_ui_init(EinaFieshta *self)
{
	// Build UI
	gtk_clutter_init(NULL, NULL);

	ClutterColor black = {0, 0, 0, 0};
	
	self->embed = gtk_clutter_embed_new();
	ClutterActor *s = gtk_clutter_embed_get_stage((GtkClutterEmbed *) self->embed);
	clutter_actor_set_size(s, 1280, 768);
	clutter_stage_set_color((ClutterStage *) s, &black);

	self->stage = fieshta_stage_new();
	clutter_actor_set_size((ClutterActor *) self->stage,  1280, 768);
	fieshta_stage_set_slots(self->stage, SLOTS);

	clutter_container_add_actor((ClutterContainer *) s, (ClutterActor *)self->stage);
	clutter_actor_show_all((ClutterActor *) s);

	// Insert streams
	LomoPlayer *lomo = eina_obj_get_lomo(self);
	gint curr = lomo_player_get_current(lomo);

	// prev streams
	gint i;
	gint slot = -1;
	for (i = curr - (SLOTS/2); i <= curr + (SLOTS/2); i++)
	{
		slot++;
		if (i < 0)
			continue;
		if (i > lomo_player_get_total(lomo) - 1)
			break;

		LomoStream *stream = lomo_player_nth_stream(lomo, i);

		gchar *path = gel_resource_locate(GEL_RESOURCE_IMAGE, "cover-default.png");
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 256, 256, TRUE, NULL);
		FieshtaStream *s = fieshta_stream_new(pb, lomo_stream_get_tag(stream, LOMO_TAG_TITLE), lomo_stream_get_tag(stream, LOMO_TAG_ARTIST));
		fieshta_stage_set_nth(self->stage, slot, (ClutterActor*) s);
		g_free(path);
		g_object_unref(pb);
	}

	GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add((GtkContainer *) w, (GtkWidget *) self->embed);
	gtk_widget_show_all(w);
}

static void
lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaFieshta *self)
{
	if ((from == lomo_player_get_total(lomo) - 1) && (to == 0))
	{
		gel_warn("Loop done");
		self->loop_done = TRUE;
	}

	// Move other slots
	gint i = 0;
	for (i = 0; i < SLOTS - 1; i++)
	{
		ClutterActor *p = fieshta_stage_get_nth(self->stage, i + 1);
		if (!p)
			continue;
		g_object_ref(p);
		fieshta_stage_set_nth(self->stage, i + 1, NULL);
		fieshta_stage_set_nth(self->stage, i , p);
		g_object_unref(p);
	}

	LomoStream *stream = lomo_player_nth_stream(lomo, (to + (SLOTS/2)) % lomo_player_get_total(lomo));
	gchar *path = gel_resource_locate(GEL_RESOURCE_IMAGE, "cover-default.png");
	GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(path, 256, 256, TRUE, NULL);
	FieshtaStream *s = fieshta_stream_new(pb, lomo_stream_get_tag(stream, LOMO_TAG_TITLE), lomo_stream_get_tag(stream, LOMO_TAG_ARTIST));
	fieshta_stage_set_nth(self->stage, SLOTS - 1, (ClutterActor *) s);
	g_free(path);
}

static void
lomo_insert_cb(LomoPlayer *lomo, LomoStream *stream, gint pos, EinaFieshta *self)
{
	if (!self->loop_done)
		return;

	gel_warn("Add with loop_done, we must jump!");
	lomo_player_hook_remove(lomo, (LomoPlayerHook) fieshta_hook);
	lomo_player_go_nth(lomo, pos, NULL);
	lomo_player_hook_add(lomo, (LomoPlayerHook) fieshta_hook, self);
	lomo_player_play(lomo, NULL);
	self->loop_done = FALSE;
}

static gboolean
fieshta_hook(LomoPlayer *lomo, LomoPlayerHookEvent ev, gpointer ret, EinaFieshta *self)
{
	// Block seek-ff
	if ((ev.type == LOMO_PLAYER_HOOK_SEEK)
		&& self->options[BLOCK_SEEK_FF]
		&& (ev.old < ev.new))
	{
		gel_warn("seek-ff blocked");
		return TRUE;
	}

	// Block seek-rew
	if ((ev.type == LOMO_PLAYER_HOOK_SEEK)
		&& self->options[BLOCK_SEEK_REW]
		&& (ev.old > ev.new))
	{
		gel_warn("seek-rew blocked");
		return TRUE;
	}

	// Block insert but at the end
	if ((ev.type == LOMO_PLAYER_HOOK_INSERT)
		&& (ev.pos != lomo_player_get_total(lomo)))
	{
		gel_warn("insert blocked");
		return TRUE;
	}

	// Block change
	if (ev.type == LOMO_PLAYER_HOOK_CHANGE)
	{
		if (ev.to == (ev.from + 1))
			return FALSE;
		if ((ev.from == lomo_player_get_total(lomo) - 1) && (ev.to == 0))
			return FALSE;
		gel_warn("change from %d to %d blocked", ev.from, ev.to);
		return TRUE;
	}

	return FALSE;
}

static void
set_fieshta_mode_activated_cb(GtkAction *action, EinaFieshta *self)
{
	eina_fieshta_set_mode(self, gtk_toggle_action_get_active((GtkToggleAction *) action));
}

EINA_PLUGIN_SPEC(fieshta,
	"0.0.1",
	"player",

	NULL,
	NULL,

	N_("Party plugin"),
	NULL,
	"fieshta.png",

	fieshta_init,
	fieshta_fini
);

