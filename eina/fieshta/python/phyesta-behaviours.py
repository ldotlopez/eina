#!/usr/bin/python

import clutter
import glib
import gobject

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
		self.w = 1280
		self.h = 768
		self.set_size(self.w, self.h)
		self.set_color((0,0,0,0))
		self.actors = []

	def set_slots(self, n):
		self.slots     = n
		self.scales    = [0] * n
		self.sizes     = [0] * n
		self.positions = [0] * n

		# [0.25, 0.125, 0.625, ...]
		base_scale = 0.25
		for i in xrange(0, (n / 2) + 1):
			sa = (n/2) + i
			sb = (n/2) - i
			self.scales[sa] = base_scale / (2**i)
			self.scales[sb] = self.scales[sa]

			self.sizes[sa] = int(self.scales[sa] * self.get_height())
			self.sizes[sb] = self.sizes[sa]

		# Calculate positions
		margin = 10
		for x in xrange(n/2 + 1, n):
			self.positions[x] = self.positions[x-1] + (self.sizes[x-1] / 2) + (self.sizes[x] / 2) + 10
			self.positions[n-x-1] = -self.positions[x]
		for i in xrange(0, self.slots):
			self.positions[i] = (self.h / 2)+ self.positions[i]

		print "Scales    ", self.scales
		print "Sizes     ", self.sizes
		print "Positions ", self.positions

	def position(self, actor, i):
		(x, y) =   (self.get_width() / 2, self.positions[i])
		(ow, oh) = actor.get_size()

		nh = self.get_height() * self.scales[i]
		nw = int(ow * (float(nh)/oh))

		if actor in self.actors:
			timeline = clutter.Timeline(fps=60, duration=1000)
			alpha = clutter.Alpha(timeline, clutter.ramp_inc_func)
			knots = (
				(1280/2, self.positions[i-1]),
				(1280/2, self.positions[i]),
				)
			print "Generate transition from %d to %d" % (i-1, i)
			print "Knots: %d", knots
			b = clutter.BehaviourPath(alpha=alpha, knots=knots)
			b.apply(actor)
			timeline.start()
			#glib.timeout_add(100, self.start_timeline_cb, timeline)
		else:
			actor.set_position(x,y)
			actor.set_scale(float(nw)/ow, float(nh)/oh)

	def start_timeline_cb(self, timeline):
		timeline.start()
		print "Playing: %d" % timeline.is_playing()
		return False

	def frame_cb(self):
		print "frame"

	def push(self, title, artist, cover):
		# Delete last actor
		while len(self.actors) >= self.slots:
			print "Deleting actor"
			actor = self.actors.pop()
			self.remove(actor)

		# Move items 0..len-1 to i+1
		for i in xrange(0, len(self.actors)):
			print "Move %d to %d" % (i, i+1)
			self.position(self.actors[i], i+1)

		# Add new stream and the end
		stream = Stream(title, artist, cover)
		self.add(stream)
		self.position(stream, 0)
		self.actors = [stream] + self.actors

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

i = 0
def timeout_cb(stage):
	global i
	data = (
		('Smell like teen spirit', 'Nirvana', '0.jpg'),
		('Masquerade', 'Ble ble', '1.jpg'),
		('Some', 'Bjork', '4.jpg'),
		('Abadi bi abade ba', 'The starting line', '2.jpg'),
		('13 steps', 'A perfect circle', '3.jpg')
	)
	item = data[i % len(data)]
	i = i+1
	print "=> %d" % (i)
	stage.push(item[0], item[1], item[2])
	return True

def main():
	stage = Phyesta()
	stage.connect("key-press-event", clutter.main_quit)

	stage.show()
	stage.set_slots(5)
	#gobject.timeout_add(500, timeout_cb, stage)
	#for i in xrange(0, len(data)):
	#	stage.add_stream(Stream(data[i][0], data[i][1], data[i][2]), i)
	#
	clutter.main()

if __name__ == '__main__':
	main()
