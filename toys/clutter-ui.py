#!/usr/bin/python

import sys
from gi.repository import Clutter, ClutterX11, Mx

class CoverWidget(Clutter.Box):
	def __init__(self, *args, **kwargs):
		Clutter.Box.__init__(self,
			layout_manager = Clutter.BinLayout(),
			width = 200, height = 200)
		self.actors    = []
		self.timelines = []

	def set_from_file(self, filename):
		try:
			w = self.get_width()
			texture = Clutter.Texture(filename = filename,
				width = 200 , height = 200,
				opacity = 0x00)
		except Exception as e:
			print repr(e)
			return

		self.add_actor(texture)

		timeline = Clutter.Timeline(duration = 1000)
		texture.animate_with_timelinev(Clutter.AnimationMode.LINEAR, timeline,
			("opacity",), (0xff,))
		timeline.connect('completed', self.timeline_completed_cb)

		self.actors.append(texture)
		self.timelines.append(timeline)

		texture.show()

	def timeline_completed_cb(self, timeline):
		try:
			index = self.timelines.index(timeline)
		except ValueError:
			print "Invalid timeline"
			return

		if index == 0:
			return
		index = index - 1
		actor = self.actors[index]
		self.remove_actor(actor)

		self.actors.pop(index)
		self.timelines.pop(index)

		#print "Removed index #%d. current items: %d" % (index, len(self.get_children()))

class Controls(Clutter.Box):
	def __init__(self, *args, **kwargs):
		Clutter.Box.__init__(self, *args, **kwargs)
		self.set_layout_manager(Clutter.FixedLayout(
			#homogeneous= False, column_spacing = 0.0,
			#orientation = Clutter.FlowOrientation.HORIZONTAL
			))

		self.buttons = dict()
		d = (
			('previous', 'media-skip-backward'),
			('playctl', 'media-playback-start'),
			('next', 'media-skip-forward')
		)
		x = 0
		for (id_, icon_name) in d:
			icon = Mx.Icon(icon_name = icon_name, icon_size = 32)

			button = Mx.Button()
			button.add_actor(icon)

			button.show_all()
			self.add_actor(button)
			self.buttons[id_] =  button
			self.buttons[id_].set_x(x)
			x = x + (32 * 11.0/6.0)
			
class App(Clutter.Stage):
	def __init__(self, covers):
		Clutter.Stage.__init__(self)

		bg_color = Clutter.Color()
		bg_color.from_string('#00000099')
		self.set_color(bg_color)
		self.set_use_alpha(True)
		self.set_size(200, 200)

		self.i = 0
		self.files = covers

		self.cover = CoverWidget(width = self.get_width(), height = self.get_height())
		self.cover.set_reactive(True)
		self.cover.show()
		self.cover.connect('button-press-event', self.click_cb)
		self.add_actor(self.cover)

		self.cover.set_from_file(self.next_cover())

		controls = Controls(width=100, height=50)
		controls.show()
		self.add_actor(controls)

	def click_cb(self, cover, ev):
		cover.set_from_file(self.next_cover())

	def next_cover(self):
		if len(self.files) == 0:
			raise Exception('No input files')

		r = self.files[self.i%len(self.files)]
		self.i = self.i + 1
		return r

if __name__ == '__main__':
	ClutterX11.set_use_argb_visual(True)
	Clutter.init([])
	app = App(sys.argv[1:])
	app.connect('destroy', lambda w: Clutter.main_quit())
	app.show()
	Clutter.main()

