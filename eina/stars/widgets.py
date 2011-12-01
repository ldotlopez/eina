from gi.repository import Gtk, Gdk, GdkPixbuf, GObject

class EinaStars(Gtk.Grid):
    __gtype_name__ = 'EinaStars'

    #
    # Handle n-stars 
    # 
    def get_n_stars(self):
        try:
            n = self.prop_n_stars
        except AttributeError:
            return 0
        return self.prop_n_stars

    def set_n_stars(self, n):
        self.prop_n_stars = n
        self.draw_stars()

    #
    # Properties
    #

    # Number of stars available
    max_stars = GObject.property(nick='max-stars', type = int,
        minimum = 1, default = 5,
        flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

    # Active stars
    n_stars = GObject.property (nick='n-stars', type = int,
        minimum = 0, default = 0,
        setter = set_n_stars, getter = get_n_stars)

    # Filename for the "active" star
    on_image  = GObject.property (nick='on-image',  type = str,
        flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

    # Filename for the "inactive" star
    off_image = GObject.property (nick='off-image', type = str,
        flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

    def __init__ (self, *args, **kwargs):
        Gtk.Grid.__init__ (self, *args, **kwargs)
        self.imgs = []
        for i in xrange(0, self.max_stars):
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

    def do_get_preferred_width(self):
	#print "do_get_preferred_width (height:%d)" % self.get_allocation().height
	return (self.max_stars * 8, self.max_stars * 8)

    def do_get_preferred_height(self):
	#print "do_get_preferred_height (width:%d)" % self.get_allocation().width
	p = self.get_parent()
	if p is None:
		return (8, 8)
	else:
		return (p.get_allocation().height, p.get_allocation().height)

    def go_get_preferred_width_for_height(self, height):
        return (height * self.max_stars, height * self.max_stars)

    def go_get_preferred_height_for_width(self, width):
        return (width / self.max_stars, width / self.max_stars)

    def draw_stars(self):
        if not self.get_realized():
            return

        for i in xrange(0, self.n_stars):
            self.imgs[i].set_from_pixbuf(self.pb_on)

        for i in xrange(self.n_stars, self.max_stars):
            self.imgs[i].set_from_pixbuf(self.pb_off)

    def size_allocate_cb(self, w, alloc):
        h = max(int(alloc.height / 2), 1)
        w = max(int((alloc.width / self.max_stars) * 0.80), 1)

        if h > w:
            h = -1
        else:
            w = -1

        self.pb_on  = GdkPixbuf.Pixbuf.new_from_file_at_scale(self.on_image,  w , h , True)
        self.pb_off = GdkPixbuf.Pixbuf.new_from_file_at_scale(self.off_image, w , h , True)

        self.draw_stars()

    def click_cb(self, w, ev):
        img = w.get_child()
        for i in xrange(0, self.max_stars):
            if img == self.imgs[i]:
                if i == 0 and (self.n_stars == 1):
                    i = -1
                self.n_stars = i + 1
                return False
        return False

# Testing code

if __name__ == '__main__':
    def notify_n_stars_cb(w, prop):
        print "Stars: %d" % w.n_stars

    Gtk.init([])
    w = Gtk.Window()
    s = EinaStars(max_stars = 7, n_stars = 2, on_image = 'star-on.svg', off_image = 'star-off.svg')
    s.connect("notify::n-stars", notify_n_stars_cb)
    s.set_hexpand(True)
    s.set_vexpand(True)

    w.add(s)
    w.show_all()
    w.connect('delete-event', lambda w,ev : Gtk.main_quit())
    Gtk.main()

