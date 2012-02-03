#!/usr/bin/python

import sys
from gi.repository import Clutter, ClutterX11, Mx

class CoverWidget(Clutter.Box):
	def __init__(self, *args, **kwargs):
		Clutter.Box.__init__(self)
		self.layout = Clutter.BinLayout()
		self.set_layout_manager(self.layout)
		self.actors    = []
		self.timelines = []

	def set_from_file(self, filename):
		try:
			w = self.get_width()
			texture = Clutter.Texture(
				filename = filename,
				sync_size = True,
				keep_aspect_ratio = True,
				opacity = 0x00)
		except Exception as e:
			print repr(e)
			return

		timeline = Clutter.Timeline(duration = 1000)
		texture.animate_with_timelinev(Clutter.AnimationMode.LINEAR, timeline,
			("opacity",), (0xff,))
		timeline.connect('completed', self.timeline_completed_cb)

		if len(self.actors) > 0:
			t = self.actors[-1]
			t.animatev(Clutter.AnimationMode.LINEAR, 1000,
				("opacity",) , (0x00,))

		self.actors.append(texture)
		self.timelines.append(timeline)

		self.layout.add(texture,
			Clutter.BinAlignment.CENTER,Clutter.BinAlignment.CENTER)
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

		self.layout = Clutter.TableLayout()
		self.set_layout_manager(self.layout)

		d = (
			('previous', 'media-skip-backward'),
			('playctl', 'media-playback-start'),
			('next', 'media-skip-forward')
		)

		self.buttons = dict()
		for index, (id_, icon_name) in enumerate(d):
			button = Mx.Button()
			button.add_actor(Mx.Icon(
				icon_name = icon_name,
				icon_size = 32))
			button.show_all()

			self.layout.pack(button, index, 0)
			self.buttons[id_] =  button

class App(Clutter.Stage):
	def __init__(self, covers):
		Clutter.Stage.__init__(self,
			use_alpha = True,
			user_resizable = True,
			min_height = 200,
			min_width  = 200)

		self.i = 0
		self.files = covers

		bg_color = Clutter.Color()
		bg_color.from_string('#000000ff')
		self.set_color(bg_color)

		# Setup main container
		self.box = Clutter.Box(layout_manager = Clutter.BinLayout())
		self.box.add_constraint(Clutter.BindConstraint.new(self, Clutter.BindCoordinate.SIZE, 0.0))
		self.box.show()
		self.add_actor(self.box)

		self.box_layout = self.box.get_layout_manager()

		# Setup cover (or background)
		self.cover = CoverWidget()
		self.cover.set_reactive(True)
		self.cover.set_from_file(self.next_cover())

		self.cover.connect('button-press-event', self.click_cb)
		self.cover.show()
		self.box_layout.add(self.cover, Clutter.BinAlignment.FILL, Clutter.BinAlignment.FILL)

		# Setup controls
		self.controls = Controls(opacity = 0x00)

		#ctls_h_const = Clutter.BindConstraint.new(self, Clutter.BindCoordinate.HEIGHT, 0.0)
		#self.controls.add_constraint(ctls_h_const)

		self.controls.show()
		self.box_layout.add(self.controls, Clutter.BinAlignment.CENTER, Clutter.BinAlignment.END)

		self.connect('enter-event', self.fade_in)
		self.connect('leave-event', self.fade_out)

	def fade_in(self, actor, ev):
		self.controls.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0xff,))

	def fade_out(self, actor, ev):
		self.controls.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0x00,))

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

