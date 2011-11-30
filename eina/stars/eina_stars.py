#!/usr/bin/python

from gi.repository import Gtk, Gdk, GdkPixbuf, GObject

class Stars(Gtk.Grid):
	__gtype_name__ = 'EinaStars'

	def get_n_stars(self):
		return self.prop_n_stars

	def set_n_stars(self, n):
		self.prop_n_stars = n

		if not self.get_realized():
			self.notify("n-stars")
			return

		alloc = self.get_allocation()
		h = int(alloc.height / 2)
		w = int((alloc.width / 5) *0.80)

		if h > w:
			h = -1
		else:
			w = -1

		pb_on = GdkPixbuf.Pixbuf.new_from_file_at_scale('star-on.svg', w , h , True)
		pb_off = GdkPixbuf.Pixbuf.new_from_file_at_scale('star-off.svg', w , h , True)

		for i in xrange(0, self.n_stars):
			self.imgs[i].set_from_pixbuf(pb_on)

		for i in xrange(self.n_stars, 5):
			self.imgs[i].set_from_pixbuf(pb_off)

		self.notify("n-stars")

	n_stars = GObject.property (nick='n-stars', type = int,
		minimum = 0, maximum = 5, default = 0,
		setter = set_n_stars, getter = get_n_stars)

	def __init__ (self, *args, **kwargs):
		Gtk.Grid.__init__ (self, *args, **kwargs)
		self.imgs = []
		for i in xrange(0, 5):
			img = Gtk.Image.new()

			ev = Gtk.EventBox()
			ev.add(img)
			ev.connect('button-press-event', self.click_cb)

			self.attach (ev, i, 0, 1, 1)
			self.imgs.append(img)
			img.show ()

		self.set_column_homogeneous(True)
		self.set_row_homogeneous(True)
		self.connect('size-allocate', self.size_allocate_cb)

	def size_allocate_cb(self, w, alloc):
		self.n_stars = self.n_stars

	def click_cb(self, w, ev):
		img = w.get_child()
		for i in xrange(0, 5):
			if img == self.imgs[i]:
				if i == 0 and (self.n_stars == 1):
					i = -1
				self.n_stars = i + 1
				return False
		return False

if __name__ == '__main__':
	Gtk.init([])
	w = Gtk.Window()
	s = Stars(n_stars = 2)
	s.set_hexpand(True)
	s.set_vexpand(True)

	w.add(s)
	w.show_all()
	w.connect('delete-event', lambda w,ev : Gtk.main_quit())
	Gtk.main()

