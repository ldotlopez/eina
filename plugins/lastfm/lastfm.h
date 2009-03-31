/*
 * plugins/lastfm/lastfm.h
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

#ifndef _PLUGIN_LASTFM_H
#define _PLUGIN_LASTFM_H

#define GEL_DOMAIN "Eina::Plugin::LastFM"
#define EINA_PLUGIN_NAME "LastFM"
#define EINA_PLUGIN_DATA_TYPE LastFM

#include <eina/eina-plugin.h>
#include "submit.h"
#include "artwork.h"
#if HAVE_WEBKIT
#include "webview.h"
#endif

G_BEGIN_DECLS

typedef struct {
	GtkWidget *configuration_widget;
	gchar *daemon_path;
	gchar *client_path;

	// Subplugins
	struct _LastFMSubmit  *submit;
	struct _LastFMArtwork *artwork;
#if HAVE_WEBKIT
	struct _LastFMWebView *webview;
#endif
} LastFM;

G_END_DECLS

#endif
