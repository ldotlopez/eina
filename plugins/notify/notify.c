/*
 * plugins/notify/notify.c
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

#define GEL_DOMAIN "Eina::Plugin::Notify"
#include <gmodule.h>
#include <libnotify/notify.h>
#include <gel/gel.h>
#include <eina/base.h>
#include "notify.h"

struct _EinaNotify {
	EinaBase parent;
	NotifyNotification *notification;
#if GTK_CHECK_VERSION(2, 9, 2)
	GtkStatusIcon *status_icon;
#endif
};

void on_notify_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaNotify *self);

/*
 * Init/Exit functions 
 */
G_MODULE_EXPORT gboolean eina_notify_init(GelHub *hub, gint argc, gchar *argv[]) {
	EinaNotify *self;

	self = g_new0(EinaNotify, 1);
	if(!eina_base_init((EinaBase *) self, hub, "notify", EINA_BASE_NONE)) {
		gel_error("Cannot create component");
		return FALSE;
	}

	if (!notify_init("eina")) {
		return FALSE;
	}

	g_signal_connect(LOMO(self), "change",
		G_CALLBACK(on_notify_lomo_change), self);

	return TRUE;
}

G_MODULE_EXPORT gboolean eina_notify_exit(EinaNotify *self) {
	notify_uninit();
	eina_base_fini((EinaBase *) self);
	return TRUE;
}

void on_notify_lomo_change(LomoPlayer *lomo, gint from, gint to, EinaNotify *self) {
	const LomoStream *stream;
	gchar *title = NULL, *artist = NULL, *album = NULL;
	gchar *body = NULL;

	stream = lomo_player_get_stream(lomo);
	title  = lomo_stream_get_tag(stream, LOMO_TAG_TITLE);
	artist = lomo_stream_get_tag(stream, LOMO_TAG_ARTIST);
	album  = lomo_stream_get_tag(stream, LOMO_TAG_ALBUM);

	if (self->notification)
		notify_notification_close(self->notification, NULL);

	body = g_strdup_printf("%s by %s from %s", title, artist, album);
	self->notification = notify_notification_new(
		_("Now playing..."),
		body, NULL, NULL);

	notify_notification_show(self->notification, NULL);

	g_free(title);
	g_free(artist);
	g_free(album);
}

G_MODULE_EXPORT GelHubSlave notify_connector = {
	"notify",
	(GelHubSlaveInitFunc) &eina_notify_init,
	(GelHubSlaveExitFunc) &eina_notify_exit
};

