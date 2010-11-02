/*
 * eina/dbus/lomo-player-instropection.xml.h
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

#ifndef __LOMO_PLAYER_INSTROSPECTION_XML_H
#define __LOMO_PLAYER_INSTROSPECTION_XML_H

static const gchar *lomo_player_instrospection_data =
"<node name='/net/sourceforge/Eina'>"
"<interface name='net.sourceforge.Eina.LomoPlayer'>"
"	<annotation name='org.freedesktop.DBus.GLib.CSymbol' value='lomo_player'/>"
"	<!--"
"	Getters and setters"
"	-->"
"	<!-- auto-play -->"
"	<method name='SetAutoPlay'>"
"		<arg type='b' direction='in' />"
"	</method>"
"	<method name='GetAutoPlay'>"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<!-- auto-parse -->"
"	<method name='SetAutoParse'>"
"		<arg type='b' direction='in' />"
"	</method>"
"	<method name='GetAutoParse'>"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"

"	<!--"
"	API Functions"
"	-->"
"	<method name='Play'>"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<method name='Pause'>"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<method name='PlayUri'>"
"		<arg type='s' direction='in' />"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"

"	<!-- get and set state -->"
"	<method name='GetState'>"
"		<arg type='i' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<method name='SetState'>"
"		<arg type='i' direction='in' />"
"	</method>"

"	<!-- tell and seek position and get length -->"
"	<method name='Tell'>"
"		<arg type='i' direction='in' name='format' />"
"		<arg type='x' direction='out'>"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<method name='Seek'>"
"		<arg type='i' direction='in' name='format' />"
"		<arg type='x' direction='in' name='to'/>"
"		<arg type='b' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"
"	<method name='Length'>"
"		<arg type='i' direction='in' name='format' />"
"		<arg type='x' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"

"	<!-- Volume and mute -->"
"	<method name='GetVolume'>"
"		<arg type='i' direction='out'><annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' /></arg>"
"	</method>"
"	<method name='SetVolume'>"
"		<arg type='i' direction='in' />"
"		<arg type='b' direction='out'><annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' /></arg>"
"	</method>"

"	<method name='GetMute'>"
"		<arg type='b' direction='out'><annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' /></arg>"
"	</method>"
"	<method name='SetMute'>"
"		<arg type='b' direction='in' name='mute' />"
"		<arg type='b' direction='out'><annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' /></arg>"
"	</method>"

"	<method name='GetTotal'>"
"		<arg type='i' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"

"	<method name='GetCurrent'>"
"		<arg type='u' direction='out' >"
"			<annotation name='org.freedesktop.DBus.GLib.ReturnVal' value='' />"
"		</arg>"
"	</method>"

"	<!--"
"	Signals"
"	-->"
"	<signal name='Play'  />"
"	<signal name='Pause' />"
"	<signal name='Stop'  />"

"	<signal name='Seek' >"
"		<arg type='x' name='from' />"
"		<arg type='x' name='to' />"
"	</signal>"

"	<signal name='Volume' >"
"		<arg type='i' name='volume' />"
"	</signal>"

"	<signal name='Mute'>"
"		<arg type='b' name='value' />"
"	</signal>"

"	<!--"
"	<signal name='Insert'>"
"		<arg type='o' name='stream' />"
"		<arg type='i' name='pos'/>"
"	</signal>"
"	-->"
"	<signal name='QueueClear' />"

"	<signal name='PreChange' />"

"	<signal name='Change'>"
"		<arg type='i' name='from' />"
"		<arg type='i' name='to' />"
"	</signal>"

"	<signal name='Clear' />"

"	<signal name='Repeat'>"
"		<arg type='b' name='value' />"
"	</signal>"

"	<signal name='Random'>"
"		<arg type='b' name='value' />"
"	</signal>"

"	<signal name='Eos' />"
"</interface>"

"</node>";

#endif
