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
		updates = (self.schema_version_0,)
		for i in xrange(curr_schema_version + 1, len(updates)):
			r = updates[i]()
			if r:
				self.adb.schema_set_version('stars', i)
			else:
				print "Update %d fail" % i

		box = app.get_player().get_plugins_area()

		p = os.path.dirname(__file__)
		self.stars = EinaStars(
			on_image = os.path.join(p, "star-on.svg"),
			off_image = os.path.join(p, "star-off.svg"))
		self.stars.set_vexpand(True)
		self.stars.set_hexpand(False)
		self.stars.connect('notify::n-stars', self.stars_notify_n_stars_cb)
		self.stars.show_all()
		box.pack_start(self.stars, True, True, 0)
		return True

	def do_deactivate(self, app):
		box = app.get_player().get_plugins_area()
		box.remove(self.grid)
		return True

	def stars_notify_n_stars_cb(self, w, prop_spec):
		stream = self.lomo.get_nth_stream(self.lomo.get_current())
		if stream is None:
			return

		data = { 'sid' : self.adb.lomo_stream_get_sid(stream), 'rating' : self.stars.n_stars }
		self.adb.query_exec_raw("INSERT OR REPLACE INTO stars VALUES(%(sid)d, %(rating)d)" % data)

	def lomo_change_cb(self, lomo, f, t):
		stream = self.lomo.get_nth_stream(t)
		if stream is None:
			return

		sid = self.adb.lomo_stream_get_sid(stream)
		if sid < 0:
			return

		res = self.adb.query_raw("SELECT rating FROM stars WHERE sid=%d" % (sid))
		if not res.step():
			n_stars = 0
		else:
			n_stars = res.get_value(0, int)

		self.stars.disconnect_by_func(self.stars_notify_n_stars_cb)
		self.stars.n_stars = n_stars
		self.stars.connect('notify::n-stars', self.stars_notify_n_stars_cb)

