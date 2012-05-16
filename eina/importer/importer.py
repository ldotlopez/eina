import os, stat, sys, urllib
from gi.repository import GObject, Lomo, Eina, Gtk

ui_mng_str = """
<ui>
  <menubar name='Main' >
	<menu name='Tools' action='plugins-menu' >
	  <menuitem name='Bulk import' action='bulk-import-action' />
	</menu>
  </menubar>
</ui>
"""

class Importer:
	EXTS = "mp3 ogg wav wma".split(" ")

	def __init__(self, adb, *args, **kwargs):
		self._streams = set()
		self._adb = adb
		self._total = 0
		self._done  = 0
		self._parser = Lomo.MetadataParser()
		self._parser.connect('all-tags', self._on_all_tags)
		if kwargs.has_key('extensions'):
			Importer.EXTS = kwargs['extensions']

	def _on_all_tags(self, parser, stream):
		# print "== Tags for %s" % os.path.basename(stream.get_tag('uri'))
		self._streams.remove(stream)
		tags = stream.get_tags()
		self._done = self._done + 1
		print "%d de %d" % (self._done, self._total)

		if (len(tags) == 1) and (tags[0] == 'uri'):
			return

		self._adb.lomo_stream_attach_sid(stream)
		sid = self._adb.lomo_stream_get_sid(stream)

		return

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
			except OSError as e:
				print repr(e)
				continue

			if stat.S_ISLNK(s.st_mode):
				continue

			if stat.S_ISDIR(s.st_mode):
				self.scan([os.path.join(path, child) for child in os.listdir(path) if child[0] != '.' ], False)

			elif stat.S_ISREG(s.st_mode) and (os.path.splitext(path)[1][1:].lower() in Importer.EXTS):
				stream = Lomo.Stream(uri = 'file://' + urllib.pathname2url(path))
				self._total = self._total + 1
				self._streams.add(stream)
				self._parser.parse(stream, Lomo.MetadataParserPrio.DEFAULT)

class ImporterPlugin(GObject.Object, Eina.Activatable):
	__gtype_name__ = 'EinaImporterPlugin'

	application = GObject.property (type = GObject.Object)

	def __init__(self):
		GObject.Object.__init__ (self)

	def do_activate(self, app):
		self._app = app

		self._action = Gtk.Action(name = 'bulk-import-action',
			label = _(u'Bulk import'),
			tooltip = _(u"Import lots of file"),
			stock_id = None)
		self._action.connect('activate', self._on_action_activate)

		ag = self._app.get_window_action_group()
		ag.add_action(self._action)

		ui_mng = self._app.get_window_ui_manager()
		self._ui_merge_id = ui_mng.add_ui_from_string(ui_mng_str)
		ui_mng.ensure_update()

		return True

	def do_deactivate(self, app):
		return True

		#self._importer = Importer(app.get_adb())
		# self._importer.scan((GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_MUSIC),))

	def _on_action_activate(self, action):
		dialog = Gtk.FileChooserDialog(title = _(u'Choose the directory to import'),
		parent = self._app.get_window(),
		action = Gtk.FileChooserAction.SELECT_FOLDER)
		dialog.add_button(Gtk.STOCK_OK, 1)

		code = dialog.run()
		if code == 1:
			dialog.hide()
			uri = dialog.get_filename()
			if uri:
				importer = Importer(self._app.get_adb())
				importer.scan((uri,))
		dialog.destroy()

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

