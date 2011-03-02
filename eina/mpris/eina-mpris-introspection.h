/*
 * eina/mpris/eina-mpris-introspection.h
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

#ifndef _EINA_MPRIS_INTROSPECTION_H
#define _EINA_MPRIS_INTROSPECTION_H

static const gchar *_eina_mpris_introspection =
"<node name='/org/mpris/MediaPlayer2'>"
"  <interface name='org.mpris.MediaPlayer2'>"
"    <method name='Raise' />"
"    <method name='Quit'  />"

"    <property name='CanQuit'             type='b'  access='read' />"
"    <property name='CanRaise'            type='b'  access='read' />"
"    <property name='HasTrackList'        type='b'  access='read' />"
"    <property name='Identity'            type='s'  access='read' />"
"    <property name='DesktopEntry'        type='s'  access='read' />"
"    <property name='SupportedUriSchemes' type='as' access='read' />"
"    <property name='SupportedMimeTypes'  type='as' access='read' />"
"  </interface>"

"  <interface name='org.mpris.MediaPlayer2.Player'>"
"    <method name='Next'      />"
"    <method name='Previous'  />"
"    <method name='Pause'     />"
"    <method name='PlayPause' />"
"    <method name='Stop'      />"
"    <method name='Play'      />"
"    <method name='Seek'>"
"      <arg name='Offset' type='x' direction='in'/>"
"    </method>"
"    <method name='SetPosition'>"
"      <arg name='TrackId'  type='o' direction='in' />"
"      <arg name='Position' type='x' direction='in' />"
"    </method>"
"    <method name='OpenUri' >"
"      <arg name='Uri' type='s' direction='in' />"
"    </method>"

"    <signal name='Seeked' >"
"      <arg name='Position' type='x' />"
"    </signal>"

"    <property name='PlaybackStatus' type='s' access='read' />"
"    <property name='LoopStatus'     type='s' access='readwrite' />"
"    <property name='Rate'           type='d' access='readwrite' />"
"    <property name='Shuffle'        type='b' access='readwrite' />"
"    <property name='Metadata'       type='a{sv}' access='read' />"
"    <property name='Volume'         type='d' access='readwrite' />"
"    <property name='Position'       type='x' access='read' />"
"    <property name='MinimumRate'    type='d' access='read' />"
"    <property name='MaximumRate'    type='d' access='read' />"
"    <property name='CanGoNext'      type='b' access='read' />"
"    <property name='CanGoPrevious'  type='b' access='read' />"
"    <property name='CanPlay'        type='b' access='read' />"
"    <property name='CanPause'       type='b' access='read' />"
"    <property name='CanSeek'        type='b' access='read' />"
"    <property name='CanControl'     type='b' access='read' />"
"  </interface>"
"</node>";

#endif
