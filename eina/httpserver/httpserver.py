from gi.repository import GObject, Lomo, Eina

import simplejson as json

import socket
import threading

import SocketServer
import BaseHTTPServer

lomo = None

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


class ThreadedHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_GET(self):
		global lomo

		self.send_response(200)
		self.send_header('Content-type','text/plain')
		self.end_headers()
		self.wfile.write(repr(serialize_player(lomo)))
		self.wfile.write('\n')
		return

class ThreadingHTTPServer(SocketServer.ThreadingMixIn, BaseHTTPServer.HTTPServer):
	pass

class HTTPServerPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'HTTPServerPlugin'

	application = GObject.property(type=GObject.Object)

	def do_activate(self, app):
		global lomo 
		lomo = app.get_lomo()
		self.httpd    = ThreadingHTTPServer (("", 8080), ThreadedHTTPRequestHandler)
		server_thread = threading.Thread(target=self.httpd.serve_forever)
		server_thread.setDaemon(True)
		server_thread.start()
		return True

	def do_deactivate(self, app):
		self.http.shutdown()
		return True

