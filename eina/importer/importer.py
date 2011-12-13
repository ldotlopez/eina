import os, stat, sys, urllib, pyinotify
from gi.repository import GObject, Lomo, Eina

class Importer:
	EXTS = "mp3 ogg wav wma".split(" ")

	def __init__(self, adb, *args, **kwargs):
		self._adb = adb
		self._parser = Lomo.MetadataParser()
		self._parser.connect('all-tags', self._on_all_tags)
		if kwargs.has_key('extensions'):
			Importer.EXTS = kwargs['extensions']

	def _on_all_tags(self, parser, stream):
		print "== Tags for %s" % os.path.basename(stream.get_tag('uri'))
		tags = stream.get_tags()
		if (len(tags) == 1) and (tags[0] == 'uri'):
			return


		self._adb.lomo_stream_attach_sid(stream)
		sid = self._adb.lomo_stream_get_sid(stream)
		print "   SID: %d" %sid
		self._adb.query_exec_raw("BEGIN TRANSACTION")
		for tag in stream.get_tags():
			try:
				v = stream.get_tag(tag)
				v_str = str(v)
			except TypeError:
				pass
			else:
				if (type(v) in (int, str, bool)) and (tag != 'uri'):
					self._adb.query_exec_raw("INSERT OR REPLACE into metadata (sid,key,value) VALUES(%d,'%s','%s')" %
						(sid, tag, str(v)))
					print "INSERT OR REPLACE into metadata (sid,key,value) VALUES(%d,'%s','%s')" % \
						(sid, tag, str(v))
		self._adb.query_exec_raw("END TRANSACTION")

	def scan(self, paths, top = True):
		for path in paths:
			try:
				s = os.lstat(path)
			except OSError:
				continue

			if stat.S_ISLNK(s.st_mode):
				continue

			if stat.S_ISDIR(s.st_mode):
				self.scan([os.path.join(path, child) for child in os.listdir(path) if child[0] != '.' ], False)

			elif stat.S_ISREG(s.st_mode) and (os.path.splitext(path)[1][1:].lower() in Importer.EXTS):
				self._parser.parse(Lomo.Stream(uri = 'file://' + urllib.pathname2url(path)), Lomo.MetadataParserPrio.DEFAULT)

class ImporterPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'EinaImporterPlugin'

	application = GObject.property (type = GObject.Object)

	def __init__(self):
		GObject.Object.__init__ (self)

	def do_activate(self, app):
		self._importer = Importer(app.get_adb())
		self._importer.scan((GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_MUSIC),))
		return True

	def do_deactivate(self, app):
		return True

if __name__ == '__main__':
	def do_run():
		i = Importer(extensions = ('mp3',))
		i.scan(sys.argv)
		return False

	try:
		Lomo.init(0, '')
		Gtk.init([])
		w = Gtk.Window()
		w.show_all()
		w.connect('delete-event', lambda w, ev: Gtk.main_quit())
		GLib.timeout_add(1, do_run)
		Gtk.main()
	except KeyboardInterrupt:
		sys.exit(0)

