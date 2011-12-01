import os
from gi.repository import Gtk, Eina, GObject
from widgets import EinaStars
import gettext
gettext.textdomain("eina")

class StarsPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'EinaStarsPlugin'

	application = GObject.property (type = GObject.Object)

	def __init__(self):
		GObject.Object.__init__ (self)

	def do_activate(self, app):
		box = app.get_player().get_plugins_area()

		p = os.path.dirname(__file__)
		stars = EinaStars(
			on_image = os.path.join(p, "star-on.svg"),
			off_image = os.path.join(p, "star-off.svg"))
		stars.show_all()
	
		box.pack_start(stars, True, True, 0)
		return True

	def do_deactivate(self, app):
		box = app.get_player().get_plugins_area()
		box.remove(self.grid)
		return True

