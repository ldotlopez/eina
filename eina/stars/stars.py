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

	def schema_version_0(self):
		r = self.adb.query_block_exec(
			(
				"""CREATE TABLE stars (
					sid INTEGER NOT NULL,
					rating INTEGER NOT NULL DEFAULT 0,
					CONSTRAINT stars_pk PRIMARY KEY(sid),
					CONSTRAINT stars_fk_sid FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE
				); """,
			)
		)
		return r

	def do_activate(self, app):
		self.adb = app.get_adb()
		self.lomo = app.get_lomo()
		self.lomo.connect('change', self.lomo_change_cb)

		curr_schema_version = self.adb.schema_get_version('stars')
		print "Initial schema version: %d" % curr_schema_version
		updates = (self.schema_version_0,)
		for i in xrange(curr_schema_version + 1, len(updates)):
			r = updates[i]()
			if r:
				self.adb.schema_set_version('stars', i)
			else:
				print "Update %d fail" % i

		box = app.get_player().get_plugins_area()

		p = os.path.dirname(__file__)
		stars = EinaStars(
			on_image = os.path.join(p, "star-on.svg"),
			off_image = os.path.join(p, "star-off.svg"))
		stars.set_vexpand(True)
		stars.set_hexpand(False)
		stars.connect('notify::n-stars', self.stars_notify_n_stars_cb)
		stars.show_all()
		box.pack_start(stars, True, True, 0)
		return True

	def do_deactivate(self, app):
		box = app.get_player().get_plugins_area()
		box.remove(self.grid)
		return True

	def stars_notify_n_stars_cb(self, w, prop_spec):
		stream = self.lomo.get_nth_stream(self.lomo.get_current())
		if stream is None:
			return

		sid = self.adb.lomo_stream_get_sid(stream)
		n_stars = w.get_n_stars()

		q = "INSERT OR REPLACE INTO stars VALUES(%(sid)d, %(rating)d);" % {'sid' : sid, 'rating' : n_stars}
		self.adb.query_exec_raw(q)

		print "SID %d got %d stars" % (sid, n_stars)
		print q

	def lomo_change_cb(self, lomo, f, t):
		stream = self.lomo.get_nth_stream(t)
		if stream is None:
			return
		sid = self.adb.lomo_stream_get_sid(stream)
		if sid < 0:
			return

		print "new sid: ", sid

		q = "SELECT rating FROM stars WHERE sid=%d" % (sid)
		res = self.adb.query_raw(q)

