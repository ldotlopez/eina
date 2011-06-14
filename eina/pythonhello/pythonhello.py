import gobject
from gi.repository import Eina
from gi.repository import Lomo

class PythonHelloPlugin(gobject.GObject, Eina.Activatable):
	__gtype_name__ = 'PythonHelloPlugin'

	# object = gobject.property(type=gobject.GObject)

	def do_activate(self, app):
		print "PythonHelloPlugin.do_activate: app:%s" % (repr(app))
		return True

	def do_deactivate(self, app):
		print("PythonHelloPlugin.do_deactivate", repr(app))
		return True
