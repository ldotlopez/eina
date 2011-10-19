import gi
from gi.repository import GObject, Lomo, Eina

from urlparse import urlparse, parse_qs
import simplejson as json
import socket
import threading

import SocketServer
import BaseHTTPServer

lomo = None

def InvalidParams(Exception): pass

class EinaJSONEncoder(json.JSONEncoder):
	def _object(self, obj):
		ret = {}
		for pspec in GObject.list_properties(obj):
			name  = pspec.name
			value = obj.get_property(name)

			if hasattr(value, '__enum_values__'):
				value = int(value)
			ret[name] = value

		return ret

	def _player(self, player):
		return self._object(player)

	def _stream(self, stream):
		ret = self._object(stream)

		# Add all tags except uri (already a property)
		ret['tags'] = {}
		for tag in [x for x in stream.get_tags() if x != 'uri']:
			try:
				ret['tags'][tag] = stream.get_tag(tag)
			except Exception as e:
				print "[E] %s" % repr(e)

		# Add extended metadata
		ret['extended-metadata'] = {}
		for key in stream.get_extended_metadata_keys():
			try:
				ret['extended-metadata'][key] = stream.get_extended_metadata_as_string(key)
			except Exception as e:
				print "[E] %s" % repr(e)

		return ret

	def default(self, obj):
		if type(obj) == gi.repository.Lomo.Player:
			ret = self._player(obj)

		elif type(obj) == gi.repository.Lomo.Stream:
			ret = self._stream(obj)

		elif type(obj) == gi.repository.Lomo.State:
			ret = int(obj)

		elif type(obj) == gi.repository.GObject.Object:
			ret = self._object(obj)

		else:
			ret = json.JSONEncoder.default(self, obj)

		return ret

class ThreadedHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def handle_player(self, player, method_name, args):
		enc = EinaJSONEncoder()

		if method_name == 'dump':
			self.send_response(200)
			self.send_header('Content-type', 'text/plain') # really application/json
			self.end_headers()

			self.wfile.write(json.dumps(player, default = enc.default) + "\n")
			return

		else:
			try:
				resp = lomo.__getattribute__(method_name)(*args)
				self.send_response(200)
				self.send_header('Content-type', 'text/plain') # really application/json
				self.end_headers()

				self.wfile.write(json.dumps({'return': resp}, default = enc.default) + "\n")

			except Exception as e:
				return self.handle_error(-1, str(e))

	def handle_stream(self, stream):
		enc = EinaJSONEncoder()
		self.send_response(200)
		self.send_header('Content-type', 'text/plain') # really application/json
		self.end_headers()

		self.wfile.write(json.dumps(stream, default = enc.default) + "\n")

	def handle_error(self, e_code = -1, detail = ''):
		self.send_response(500)
		self.send_header('Content-type', 'text/plain') # really application/json
		self.end_headers()

		self.wfile.write(json.dumps({
			'error'  : e_code,
			'detail' : str(detail)}) + "\n")

	def do_GET(self):
		global lomo
		u = urlparse(self.path)

		try:
			(target, method) = u.path.split('/')[1:3]
		except:
			return self.handle_error(-1, 'Invalid URL')

		args = []
		try:
			args = json.loads(parse_qs(u.query)['args'][0])
		except:
			pass

		if target not in ('player','stream'):
			return self.handle_error(-1, 'Invalid target')

		# Handle player petition
		if target == 'player':
			return self.handle_player(lomo, method, args)

		# Handle stream petition
		elif target == 'stream':
			try:
				stream = lomo.get_nth_stream(int(method))
			except Exception as e:
				return self.handle_error(-1, 'Invalid stream (%s)' % str(e))

			return self.handle_stream(stream)

		else:
			raise Exception('Big error')

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
		print "HTTP server running in http://localhost:8080/"
		return True

	def do_deactivate(self, app):
		self.http.shutdown()
		return True

