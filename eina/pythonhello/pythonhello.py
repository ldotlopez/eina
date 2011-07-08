import gobject
from gi.repository import Eina

class PythonHelloPlugin(gobject.GObject, Eina.Activatable):
	__gtype_name__ = 'PythonHelloPlugin'

	application = gobject.property(type=gobject.GObject)

	def do_activate(self, app):
		print "Python test plugin, display current playlist:"
		lomo = app.get_lomo()
		for stream in lomo.get_playlist():
			print stream.get_tag('uri')
		lomo.connect('insert', self.insert_cb)
		return True

	def do_deactivate(self, app):
		print("PythonHelloPlugin.do_deactivate", repr(app))
		return True

	def insert_cb(self, lomo, stream, pos):
		print "Added at pos %s: %s" % (pos, stream.get_tag('uri'))
