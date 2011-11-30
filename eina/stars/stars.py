from gi.repository import Gtk, Eina, GObject

import gettext
gettext.textdomain("eina")

class StarsPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'EinaStarsPlugin'

	application = GObject.property (type = GObject.Object)

	def __init__(self):
		GObject.Object.__init__ (self)

	def do_activate(self, app):
		self.grid = Gtk.Grid()
		for i in xrange(0, 5):
			img = Gtk.Image.new_from_stock(Gtk.STOCK_OK, Gtk.IconSize.SMALL_TOOLBAR)
			img.set_valign(Gtk.Align.CENTER)
			img.set_vexpand(True)
			self.grid.attach(img, i, 0, 1, 1)
		self.grid.show_all()
		box = app.get_player().get_plugins_area()
		box.pack_start(self.grid, True, True, 0)
		return True

	def do_deactivate(self, app):
		box = app.get_player().get_plugins_area()
		box.remove(self.grid)
		return True

