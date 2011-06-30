# -*- coding: utf-8 -*-

# pythonconsole.py -- plugin object
#
# Copyright (C) 2006 - Steve Frécinaux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Parts from "Interactive Python-GTK Console" (stolen from epiphany's console.py)
#     Copyright (C), 1998 James Henstridge <james@daa.com.au>
#     Copyright (C), 2005 Adam Hooper <adamh@densi.com>
# Bits from gedit Python Console Plugin
#     Copyrignt (C), 2005 Raphaël Slinckx
#
# The Totem project hereby grant permission for non-gpl compatible GStreamer
# plugins to be used and distributed together with GStreamer and Totem. This
# permission are above and beyond the permissions granted by the GPL license
# Totem is covered by.
#
# Monday 7th February 2005: Christian Schaller: Add exception clause.
# See license_change file for details.

from console import PythonConsole

__all__ = ('PythonConsole', 'OutFile')

from gi.repository import Peas
from gi.repository import Gtk
from gi.repository import Eina
from gi.repository import Lomo
import gobject

import gettext
gettext.textdomain("eina")

ui_mng_str = """
<ui>
  <menubar name='Main' >
    <menu name='Tools' action='plugins-menu' >
      <menuitem name='Python console' action='python-console-action' />
    </menu>
  </menubar>
</ui>
"""


class PythonConsolePlugin(gobject.GObject, Eina.Activatable):
	__gtype_name__ = 'EinaPythonConsolePlugin'

	object = gobject.property(type = gobject.GObject)

	def __init__(self):
		self.app    = None
		self.window = None
		self.action = None
		self.ui_merge_id = 0

	def do_activate(self, app):
		self.app = app

		self.action = Gtk.ToggleAction(name = 'python-console-action', label = _(u'_Python Console'), tooltip = _(u"Show Totem's Python console"), stock_id = 'gnome-mime-text-x-python')
		self.action.connect('activate', self.console_action_activated_cb)

		self.app.get_window_action_group().add_action(self.action)

		ui_mng = self.app.get_window_ui_manager()

		self.ui_merge_id = ui_mng.add_ui_from_string(ui_mng_str)
		ui_mng.ensure_update()

		return True

	def do_deactivate(self, app):
		if self.window is not None:
			self.destroy_console()

		ui_mng = self.app.get_window_ui_manager()
		ui_mng.remove_ui(self.ui_merge_id)
		ui_mng.ensure_update()

		self.app.get_window_action_group().remove_action(self.action)

		return True

	def console_action_activated_cb(self, action):
		if action.get_active():
			self.show_console()
		else:
			self.destroy_console()

	def show_console(self):
		if not self.window:
			console = PythonConsole(namespace = { '__builtins__' : __builtins__,
					'Lomo' : Lomo,
					'Eina' : Eina,
					'app'  : self.app,
					'core' : self.app.get_lomo()},
				destroy_cb = self.destroy_console)

			console.set_size_request(600, 400)

			v = {
				'Eina' : _(u"Eina namespace")          , 
				'Lomo' : _(u"Lomo namespace")          ,
				'app'  : _(u"EinaApplication instance"),
				'core' : _(u"LomoPlayer instance")     }

			for (k,v) in v.iteritems():
				console.eval('print "You can access %s via \'%s\'"' % (v, k))

			self.window = Gtk.Window()
			self.window.set_title(_(u'Eina Python Console'))
			self.window.add(console)
			self.window.connect('destroy', self.destroy_console)
			self.window.show_all()
		else:
			self.window.show_all()
			self.window.grab_focus()
		self.action.set_active(True)

	def destroy_console(self, *args):
		self.action.set_active(False)
		if self.window:
			self.window.destroy()
			self.window = None

