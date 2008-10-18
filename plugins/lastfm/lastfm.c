#define GEL_DOMAIN "Eina::Plugin::LastFM"
#define EINA_PLUGIN_NAME "LastFM"
#define EINA_PLUGIN_DATA_TYPE LastFmData

#include <glib.h>
#include <lomo/player.h>
#include <lomo/util.h>
#include <eina/iface.h>

typedef struct _LastFmDataSubmit {
	gint64 secs_required;
	gint64 secs_played;
	gint64 check_point;
	GtkWidget *configuration_widget;
} LastFmDataSubmit;

typedef struct LastFmData {
	LastFmDataSubmit *submit;
} LastFmData;

// --
// Submit sub-plugin
// --
static void
lastfm_submit_lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self);
static void
lastfm_submit_lomo_state_change_cb(LomoPlayer *lomo, EinaPlugin *self);
static void
lastfm_submit_lomo_eos_cb(LomoPlayer *lomo, EinaPlugin *self);

gboolean lastfm_submit_init(EinaPlugin *self, GError **error)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);

	gchar *ui_path   = NULL;
	gchar *icon_path = NULL;

	GtkBuilder *builder = NULL;
	GtkWidget  *icon = NULL, *label = NULL, *widget = NULL;

	data->submit = g_new0(LastFmDataSubmit, 1);

	ui_path = eina_plugin_build_resource_path(self, "lastfm.ui");
	builder = gtk_builder_new();

	if (gtk_builder_add_from_file(builder, ui_path, NULL) == 0)
		goto lastfm_submit_init_fail;
	
	if ((widget = GTK_WIDGET(gtk_builder_get_object(builder, "main-window"))) == NULL)
		goto lastfm_submit_init_fail;
	if ((widget = gtk_bin_get_child(GTK_BIN(widget))) == NULL)
		goto lastfm_submit_init_fail;

	icon_path = eina_plugin_build_resource_path(self, "lastfm.png");
	if ((icon = gtk_image_new_from_file(icon_path)) == NULL)
		goto lastfm_submit_init_fail;
	
	label = gtk_label_new(_("LastFM"));

	eina_plugin_add_configuration_widget(self, GTK_IMAGE(icon), GTK_LABEL(label), widget);
	data->submit->configuration_widget = widget;

	eina_plugin_attach_events(self,
		"change", lastfm_submit_lomo_change_cb,
		"play",   lastfm_submit_lomo_state_change_cb,
		"pause",  lastfm_submit_lomo_state_change_cb,
		"stop",   lastfm_submit_lomo_state_change_cb,
		"eos",    lastfm_submit_lomo_eos_cb,
		NULL);
	return TRUE;

lastfm_submit_init_fail:
	gel_free_and_invalidate(ui_path,      NULL, g_free);
	gel_free_and_invalidate(icon_path,    NULL, g_free);
	gel_free_and_invalidate(builder,      NULL, g_object_unref);
	gel_free_and_invalidate(icon,         NULL, g_object_unref);
	gel_free_and_invalidate(label,        NULL, g_object_unref);
	gel_free_and_invalidate(widget,       NULL, g_object_unref);
	gel_free_and_invalidate(data->submit, NULL, g_free);

	return FALSE;
}

gboolean lastfm_submit_exit(EinaPlugin *self, GError **error)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);

	eina_plugin_deattach_events(self,
		"change", lastfm_submit_lomo_change_cb,
		"play",   lastfm_submit_lomo_state_change_cb,
		"pause",  lastfm_submit_lomo_state_change_cb,
		"stop",   lastfm_submit_lomo_state_change_cb,
		"eos",    lastfm_submit_lomo_eos_cb,
		NULL);
	eina_plugin_remove_configuration_widget(self, data->submit->configuration_widget);

	g_free(data->submit);
	return TRUE;
}

static void
lastfm_submit_reset_count(EinaPlugin *self)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);

	eina_iface_warn("Reset counters");
	data->submit->secs_required = G_MAXINT64;
	data->submit->secs_played   = 0;
	data->submit->check_point = 0;
}

static void
lastfm_submit_set_checkpoint(EinaPlugin *self, gint64 checkpoint, gboolean add_to_played)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);

	eina_iface_warn("Set checkpoint at %lld, add: %s", checkpoint, add_to_played ? "Yes" : "No");
	if (add_to_played)
		data->submit->secs_played += (checkpoint - data->submit->check_point);
	data->submit->check_point = checkpoint;
	eina_iface_warn("played: %lld", data->submit->secs_played);
}

static void
lastfm_submit_lomo_change_cb(LomoPlayer *lomo, gint from, gint to, EinaPlugin *self)
{
	lastfm_submit_reset_count(self);
}

static void
lastfm_submit_lomo_state_change_cb(LomoPlayer *lomo, EinaPlugin *self)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);
	LomoState state = lomo_player_get_state(lomo);

	switch (state)
	{
	case LOMO_STATE_STOP:
		lastfm_submit_reset_count(self);
		break;
	case LOMO_STATE_PAUSE:
		lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), TRUE);
		break;
	case LOMO_STATE_PLAY:
		if (data->submit->secs_required == G_MAXINT64)
			data->submit->secs_required = lomo_nanosecs_to_secs(lomo_player_length_time(lomo)) / 2;
		lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), FALSE);
		break;
	case LOMO_STATE_INVALID:
		break;
	}
}

static void
lastfm_submit_lomo_eos_cb(LomoPlayer *lomo, EinaPlugin *self)
{
	LastFmData *data = EINA_PLUGIN_DATA(self);
	
	LomoStream *stream;
	stream = (LomoStream *) lomo_player_get_current_stream(lomo);
	gchar *artist, *album, *title;
	gchar *cmdl, *tmp;

	lastfm_submit_set_checkpoint(self, lomo_nanosecs_to_secs(lomo_player_tell_time(lomo)), TRUE);

	if (data->submit->secs_played < data->submit->secs_required)
	{
		eina_iface_warn("Not sending stream %p, insufficient seconds played (%lld / %lld)", stream,
			data->submit->secs_played, data->submit->secs_required);
		return;
	}

	artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);
	title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	if (!artist || !title)
	{
		eina_iface_error("Cannot submit stream %p, unavailable tags", stream);
		return;
	}

	cmdl = g_strdup_printf("/usr/lib/lastfmsubmitd/lastfmsubmit --artist \"%s\" --title \"%s\" --length %lld", artist, title, data->submit->secs_required * 2);
	if (album)
	{
		tmp = g_strdup_printf("%s --album \"%s\"", cmdl, album);
		g_free(cmdl);
		cmdl = tmp;
	}
	eina_iface_warn("EXEC %lld/%lld: '%s'", data->submit->secs_played, data->submit->secs_required, cmdl);
	g_spawn_command_line_async(cmdl, NULL);
}

// --
// Main plugin
// --
gboolean
lastfm_init(EinaPlugin *self, GError **error)
{
	self->data = g_new0(LastFmData, 1);

	if (!lastfm_submit_init(self, error))
		return FALSE;

	return TRUE;
}

gboolean
lastfm_exit(EinaPlugin *self, GError **error)
{
	if (!lastfm_submit_exit(self, error))
		return FALSE;

	g_free(self->data);
	return TRUE;
}

G_MODULE_EXPORT EinaPlugin lastfm_plugin = {
	EINA_PLUGIN_SERIAL,
	N_("Lastfm integration"),
	"0.0.1",
	N_("Lastfm integration"),
	N_("Lastfm integration:"
	"· Submit played streams to last.fm"),
	"lastfm.png",
	"xuzo <xuzo@cuarentaydos.com>",
	"http://eina.sourceforge.net/",

	lastfm_init, lastfm_exit,

	NULL, NULL

};
