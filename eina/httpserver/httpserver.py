from gi.repository import GObject, Lomo, Eina
import simplejson as json

def serialize_object(obj):
	ret = {}
	for pspec in GObject.list_properties(obj):
		try:
			name  = pspec.name
			vtype = pspec.value_type
			value = obj.get_property(name)
			
			if hasattr(value, '__enum_values__'):
				value = int(value)
			ret[name] = value

		except Exception as e:
			print "[E] %s" % repr(e)
	return ret

def serialize_player(player):
	return serialize_object(player)

class HTTPServerPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'HTTPServerPlugin'

	application = GObject.property(type=GObject.Object)

	def do_activate(self, app):
		print json.dumps(serialize_player(app.get_lomo()))
		return True

	def do_deactivate(self, app):
		return True

