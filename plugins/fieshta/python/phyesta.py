#!/usr/bin/python

import clutter

def get_apparent_size(self):
	(w, h)   = self.get_size();
	(sx, sy) = self.get_scale();
	return (int(w*sx), int(h*sy))

def set_apparent_size(self, w, h):
	(w_, h_) = self.get_size();
	self.set_scale(float(w) / float(w_), float(h) / float(h_))

class Phyesta(clutter.Stage):
	def __init__(self):
		clutter.Stage.__init__(self)
		self.set_size(1280, 768)
		self.set_color((0,0,0,0))
		self.actors = []

	def set_slots(self, n):
		self.actors = [None] * n
		self.slots  = n
		self.scales = []
		self.sizes  = []

		# [0.25, 0.125, 0.625, ...]
		for i in xrange(0, (n / 2) + 1):
			self.scales.append(float(1) / (2**(i+2)))
			self.sizes.append(int(self.scales[i] * self.get_height()))
		print "Scales ", self.scales
		print "Sizes  ", self.sizes

		# Calculate positions
		margin = 10
		self.positions = [0]
		for x in xrange(1, (n/2) + 1):
			self.positions.append(self.positions[x-1] + (self.sizes[x-1] / 2) + (self.sizes[x] / 2) + margin)
		print "Positions: ", self.positions

	def add_stream(self, actor, i):
		if (i < 0) or (i >= self.slots):
			print "Invalid index"
			return False

		if self.actors[i]:
			self.remove(self.actors[i])

		slot = (self.slots / 2) - i

		# Set apparent size
		scale = self.sizes[abs(slot)] / float(actor.get_height())
		set_apparent_size(actor, actor.get_width()*scale, self.sizes[abs(slot)])

		# Put in corrent position
		pad = self.positions[abs(slot)]
		if (slot < 0):
			pad = -pad
		actor.set_position(1280 / 2, self.get_height() / 2 + pad)
	
		self.actors[i] = actor
		self.add(self.actors[i])


class Stream(clutter.Group):
	def __init__(self, title, artist, cover):
		clutter.Group.__init__(self)

		c = clutter.Texture()
		c.set_from_file(cover)
		c.set_size(256, 256)
		c.set_position(0, 0)

		t = clutter.Label()
		t.set_text(title)
		t.set_color((255, 255, 255, 255))
		t.set_font_name('Sans Bold 70')
		t.set_position(256, 0)
		t.set_ellipsize(1)

		a = clutter.Label()
		a.set_text(artist)
		a.set_color((255, 255, 255, 255))
		a.set_font_name('Sans Bold 70')
		a.set_position(256, t.get_size()[1] )
		a.set_ellipsize(1)

		self.add(c, t, a)
		self.set_anchor_point(256, 128)

	def get_apparent_size(self):
		(w, h)   = self.get_size();
		(sx, sy) = self.get_scale();
		return (int(w*sx), int(h*sy))

	def set_apparent_size(self, w, h):
		(w_, h_) = self.get_size();
		self.set_scale(float(w) / float(w_), float(h) / float(h_))

def main():
	stage = Phyesta()
	stage.connect("key-press-event", clutter.main_quit)
	
	data = (
		('Smell like teen spirit', 'Nirvana', '0.jpg'),
		('Masquerade', 'Ble ble', '1.jpg'),
		('Some', 'Bjork', '4.jpg'),
		('Abadi bi abade ba', 'The starting line', '2.jpg'),
		('13 steps', 'A perfect circle', '3.jpg')
	)
	stage.show()
	stage.set_slots(len(data))
	for i in xrange(0, len(data)):
		stage.add_stream(Stream(data[i][0], data[i][1], data[i][2]), i)

	clutter.main()

if __name__ == '__main__':
	main()
