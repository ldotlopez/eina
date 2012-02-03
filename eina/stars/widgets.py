from gi.repository import Gtk, Gdk, GdkPixbuf, Gio, GObject, GelUI
import os

class Stars(Gtk.AspectFrame):
	__gtype_name__ = 'EinaStarsSelector'

	#
	# Handle n-stars
	#
	def get_n_stars(self):
		try:
			n = self._prop_n_stars
		except AttributeError:
			return 0
		return self._prop_n_stars

	def set_n_stars(self, n):
		self._prop_n_stars = n
		self._draw_stars()

	def set_max_stars(self, max_stars):
		self.prop_max_stars = max_stars
		self.ratio = float(1) / float(max_stars)

	def get_max_stars(self):
		try:
			return self.prop_max_stars
		except AttributeError:
			return 5

	#
	# Properties
	#

	# Number of stars available
	max_stars = GObject.property(nick='max-stars', type = int,
		minimum = 1, default = 5,
		setter = set_max_stars, getter = get_max_stars,
		flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

	# Active stars
	n_stars = GObject.property (nick='n-stars', type = int,
		minimum = 0, default = 0,
		setter = set_n_stars, getter = get_n_stars)

	# Allow zero
	allow_zero = GObject.property (nick='allow-zero', type = bool, default = True)

	# Icon name for the "active" star
	on_icon_name = GObject.property (nick='on-icon-name',  type = str,
		default = "emblem-favorite",
		flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

	# Filename for the "inactive" star
	off_image = GObject.property (nick='off-image', type = str,
		default = os.path.join(os.path.dirname(__file__), "star-off.svg"),
		flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

	def __init__ (self, *args, **kwargs):
		try:
			tmp_max_stars = kwargs['max_stars']
		except KeyError:
			tmp_max_stars = 5
		Gtk.AspectFrame.__init__ (self, obey_child = False, ratio = tmp_max_stars / float(1), **kwargs)

		grid = Gtk.Grid(column_homogeneous = True, row_homogeneous = True)
		self._imgs = []
		for i in xrange(0, self.max_stars):
			img = Gtk.Image()
			self._imgs.append(img)
			img.show ()

			ev = Gtk.EventBox()
			ev.add(img)
			ev.connect('button-press-event', self.__click_cb)

			grid.attach (ev, i, 0, 1, 1)

		self.add(grid)
		self.connect('size-allocate', self._size_allocate_cb)
		self.vexpand = False
		self.vexpand_set = True

	def _draw_stars(self):
		if not self.get_realized():
			return

		for i in xrange(0, self.n_stars):
			self._imgs[i].set_from_icon_name(self.on_icon_name, Gtk.IconSize.SMALL_TOOLBAR)

		for i in xrange(self.n_stars, self.max_stars):
			self._imgs[i].set_from_pixbuf(self._pb_off)

	def _size_allocate_cb(self, w, alloc):
		h = max(int(alloc.height / 2), 1)
		w = max(int(alloc.width / self.max_stars), 1)

		w = h = min(w, h)
		if h > w:
			h = -1
		else:
			w = -1

		self._pb_off = GdkPixbuf.Pixbuf.new_from_file_at_scale(self.off_image, w , h, True)
		self._draw_stars()

	def __click_cb(self, w, ev):
		img = w.get_child()
		for i in xrange(0, self.max_stars):
			if img == self._imgs[i]:
				if i == 0 and (self.n_stars == 1) and self.allow_zero:
					i = -1
				self.n_stars = i + 1
				return False
		return False

class Dock(GelUI.Generic):
	__gtype_name__ = 'EinaStarsDock'
	__gsignals__ = {
		'action-activate-with-amount': (GObject.SIGNAL_RUN_FIRST, GObject.TYPE_NONE,
			(Gtk.Action.__gtype__,GObject.TYPE_INT))
	}

	def __init__(self, *args, **kwargs):
		fh = open(os.path.join(os.path.dirname(__file__), 'dock.ui'))
		buff = "".join(fh.readlines())
		fh.close()

		GelUI.Generic.__init__(self, xml_string = buff)

		self._stars = Stars(
			n_stars = 3,
			vexpand = True,
			allow_zero = False)

		self._builder = self.get_builder()

		grid = self._builder.get_object('stars-grid')
		grid.attach(self._stars, 0, 1, 1, 1)
		grid.set_vexpand(False)

		for i in ('star-play-action', 'top-rated-play-action', 'random-play-action'):
			self._builder.get_object(i).connect('activate', self._action_activate_cb)

	def _get_action_amount(self, action_name):
		if action_name == 'star-play-action':
			return self._stars.n_stars

		elif action_name == 'top-rated-play-action':
			r = self._builder.get_object('top-rated-25')

		elif action_name == 'random-play-action':
			r = self._builder.get_object('random-25')

		if r is None:
			raise Exception('Unknow toggle %s' % str(toggle))

		for rr in r.get_group():
			if rr.get_active():
				return int(Gtk.Buildable.get_name(rr).split('-')[-1])

		raise Exception('Unable to find amount')

	def _action_activate_cb(self, action):
		self.emit('action-activate-with-amount', action, self._get_action_amount(action.get_name()))

# Testing code
if __name__ == '__main__':
	Gtk.init([])

	w = Gtk.Window()
	w.add(Dock(None, vexpand = False))

	w2 = Gtk.Window()
	w2.resize(200, 200)
	w2.add(Stars(n_stars = 3))

	for i in (w, w2):
		i.show_all()
		i.connect('delete-event', lambda w, ev: Gtk.main_quit())
	Gtk.main()

