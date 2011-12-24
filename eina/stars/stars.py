import os
from gi.repository import Gtk, GelUI, Eina, GObject
import widgets
import gettext
gettext.textdomain("eina")

class StarsException(Exception): pass

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

		# Create or update schema
		curr_schema_version = self.adb.schema_get_version('stars')
		updates = (self.schema_version_0,)
		for i in xrange(curr_schema_version + 1, len(updates)):
			r = updates[i]()
			if r:
				self.adb.schema_set_version('stars', i)
			else:
				print "Update %d fail" % i
				return False

		self.lomo.connect('change', self.lomo_change_cb)

		# Create and attach star widget
		p = os.path.dirname(__file__)
		self.stars = widgets.Stars(
			hexpand = False,
			xalign = 0.0,
			on_image = os.path.join(p, "star-on.svg"),
			off_image = os.path.join(p, "star-off.svg"))
		self.stars.connect('notify::n-stars', self.stars_notify_n_stars_cb)
		self.stars.show_all()

		box = app.get_player().get_plugins_area()
		box.pack_start(self.stars, True, True, 0)

		# Setup and attach dock
		self.dock_widget = widgets.Dock(app)
		self.dock_widget.show_all()

		dock = app.get_dock()
		self.dock_tab = dock.add_widget("stars",
			self.dock_widget,
			Gtk.Image.new_from_icon_name("emblem-favorite", Gtk.IconSize.MENU),
			0)

		# Actions
		self.dock_widget.connect('action-activate-with-amount', self.dock_action_activate_with_mount_cb)

		return True

	def do_deactivate(self, app):
		self.lomo.disconnect_by_func(self.lomo_change_cb)

		box = app.get_player().get_plugins_area()
		box.remove(self.stars)

		dock = app.get_dock()
		dock.remove_widget(self.dock_tab)

		self.stars = None
		self.dock_widget = None
		self.dock_tab = None

		return True

	def dock_action_activate_with_mount_cb(self, w, action, amount):
		name = action.get_name()
		if name == 'star-play-action':
			q = "SELECT sid,uri FROM stars JOIN streams USING(sid) WHERE rating >= %d ORDER BY RANDOM()" % amount
		elif name == 'top-rated-play-action':
			q = """
				SELECT a.sid,uri,rating,b.count FROM
					(select sid,rating from stars) AS a
				INNER JOIN
					(SELECT count(*) as count,sid FROM recent_plays GROUP BY (sid)) AS b
				USING (sid)
				INNER JOIN streams USING(sid)
				ORDER BY a.rating DESC
				LIMIT %d
			""" % (amount)
		elif name == 'random-play-action':
			q = "SELECT sid,uri FROM streams WHERE sid NOT IN (SELECT sid FROM stars) ORDER BY RANDOM() LIMIT %d" % amount
		else:
			raise StarsException('Unknow action: %s' % name)

		uris = []
		res = self.adb.query_raw(q)
		while res.step():
			uris.append(res.get_value(1, str))
		self.lomo.clear()
		self.lomo.insert_strv(uris, 0)

	def stars_notify_n_stars_cb(self, w, prop_spec):
		stream = self.lomo.get_nth_stream(self.lomo.get_current())
		if stream is None:
			return

		data = { 'sid' : self.adb.lomo_stream_get_sid(stream), 'rating' : self.stars.n_stars }
		if (data['rating'] == 0):
			self.adb.query_exec_raw("DELETE FROM stars WHERE sid = %(sid)d" % data)
		else:
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

