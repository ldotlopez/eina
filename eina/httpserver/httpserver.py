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
		name  = pspec.name
		value = obj.get_property(name)

		if hasattr(value, '__enum_values__'):
			value = int(value)
		ret[name] = value

	return ret

def serialize_player(player):
	return serialize_object(player)

def serialize_stream(stream):
	ret = serialize_object(stream)

	# Add all tags except uri (already a property)
	ret['tags'] = {}
	for tag in [x for x in stream.get_tags() if x != 'uri']:
		try:
			ret['tags'][tag] = stream.get_tag(tag)
		except Exception as e:
			print "[E] %s" % repr(e)

	ret['extended-metadata'] = {}
	for key in stream.get_extended_metadata_keys():
		try:
			ret['extended-metadata'][key] = stream.get_extended_metadata_as_string(key)
		except Exception as e:
			print "[E] %s" % repr(e)

	return ret


class ThreadedHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_GET(self):
		global lomo
		params = self.path.lstrip('/').split('/')

		(code, body) = (200, '')
		try:
			target = params[0]
			if len(params) > 1:
				params = params[1:]
			else:
				params = ()

			if target == 'player':
				if len(params) == 0:
					body = json.dumps(serialize_player(lomo))
				else:
					method = params[0]
					try:
						params = params[1:]
					except Exception:
						pass
					else:
						params = ()
					body = json.dumps(lomo.__getattribute__(method)(params))

			elif target == 'stream':
				n = int(params[0])
				stream = lomo.get_nth_stream(n)
				if stream is None:
					raise Exception('Invalid stream')
				body = json.dumps(serialize_stream(stream))

			else:
				code = 404
				body = json.dumps({
                          		'error'  :'unknow',
					'detail' : 'not found'})

		except Exception as e:
			self.send_response(404)
			self.end_headers()
			self.wfile.write(json.dumps({
				'error'  :'unknow',
				'detail' : repr(e)}) + "\n")

		self.send_response(200)
		self.send_header('Content-type','text/plain')
		self.end_headers()

		self.wfile.write(body + "\n")
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

