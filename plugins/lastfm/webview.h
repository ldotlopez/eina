/*
 * plugins/lastfm/webview.h
 *
 * Copyright (C) 2004-2011 Eina
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

#ifndef _PLUGIN_LASTFM_WEBVIEW_H
#define _PLUGIN_LASTFM_WEBVIEW_H

#include "lastfm.h"

G_BEGIN_DECLS

typedef struct _LastFMWebView LastFMWebView;

gboolean
lastfm_webview_init(GelApp *app, EinaPlugin *plugin, GError **error);
gboolean
lastfm_webview_fini(GelApp *app, EinaPlugin *plugin, GError **error);

G_END_DECLS

#endif

