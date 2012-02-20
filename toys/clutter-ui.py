#!/usr/bin/python

import os
import sys
import urllib
from gi.repository import Clutter, ClutterX11, Mx, Lomo, GObject

class Cover(Clutter.Box):
	__gtype_name__ = 'GlosseCover'

	#
	# Member, setter and getter for the 'lomo' property
	#
	_lomo = None
	def _get_lomo(self):
		return self._lomo

	def _set_lomo(self, lomo):
		self._lomo = lomo
		if self._lomo:
			self._lomo.connect('notify::current', lambda f, t: self._sync_from_model())
			self._sync_from_model()

	lomo = GObject.property (type = Lomo.Player,
		setter = _set_lomo, getter = _get_lomo,
		flags = GObject.PARAM_READWRITE)

	def __init__(self, *args, **kwargs):
		Clutter.Box.__init__(self)
		self.layout = Clutter.BinLayout()
		self.set_layout_manager(self.layout)
		self.actors    = []
		self.timelines = []

		if kwargs.has_key('lomo'):
			self.set_property('lomo', kwargs['lomo'])

		self.set_from_file(os.path.join(os.path.dirname(__file__), 'cover-default.png'))

	def _sync_from_model(self):
		"""
		Sync data from model
		"""
		lomo = self._get_lomo()
		stream = lomo.get_nth_stream(lomo.get_current())
		art = stream.get_extended_metadata('art-data')
		if type(art) == str and art.startswith('file:///'):
			self.set_from_file(urllib.unquote(art[7:]))

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


class Controls(Clutter.Box):
	_lomo = None

	def _set_lomo(self, lomo):
		self._lomo = lomo
		if self._lomo:
			self._lomo.connect('notify::state', lambda l,state: self.sync_from_model())
			self.sync_from_model()

	def _get_lomo(self):
		return self._lomo

	lomo = GObject.property (type = Lomo.Player,
		setter = _set_lomo, getter = _get_lomo,
		flags = GObject.PARAM_READWRITE | GObject.PARAM_CONSTRUCT_ONLY)

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
			self.buttons[id_].connect('clicked', self._button_clicked_cb)

	def sync_from_model(self):
		model = self._get_lomo()
		if model is None:
			return
		
		try:
			state = model.get_state()
			icon_name = None
			if state == Lomo.State.PLAY:
				icon_name = 'media-playback-pause'
			elif state in (Lomo.State.STOP, Lomo.State.PAUSE):
				icon_name = 'media-playback-start'
			else:
				raise Exception('Unknow state')
			self.buttons['playctl'].set_icon_name(icon_name)
		except AttributeError:
			pass

	def _button_clicked_cb(self, w):
		lomo = self._get_lomo()
		if lomo is None:
			raise('No lomo')

		if w == self.buttons['previous']:
			i = lomo.get_previous()
			if i < 0:
				return
			lomo.set_current(i)

		elif w == self.buttons['next']:
			i = lomo.get_next()
			if i < 0:
				return
			lomo.set_current(i)

		else:
			lomo.toggle_playback_state()

class App(Clutter.Stage):

	_lomo = None
	_controls = None
	_cover = None

	def _set_lomo(self, lomo):
		self._lomo = lomo

		d = { 'controls' : self._controls, 'cover' : self._cover }
		for widget in (self._cover, self._controls):
			if widget:
				widget.set_property('lomo', lomo)

	def _get_lomo(self):
		return self._lomo

	lomo = GObject.property(type = Lomo.Player,
		setter = _set_lomo, getter = _get_lomo)

	def __init__(self, uris):
		Clutter.Stage.__init__(self,
			use_alpha = True,
			user_resizable = True,
			min_height = 200,
			min_width  = 200)

		self.set_property('lomo', Lomo.Player())
		self.insert_songs(uris, 0)

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
		self._cover = Cover(lomo = self._get_lomo())
		#self.cover.set_reactive(True)
		self._cover.show()
		self.box_layout.add(self._cover, Clutter.BinAlignment.FILL, Clutter.BinAlignment.FILL)

		# Setup controls
		self._controls = Controls(lomo = self._get_lomo(), opacity = 0x00)
		self._controls.show()
		self.box_layout.add(self._controls, Clutter.BinAlignment.CENTER, Clutter.BinAlignment.END)

		self.connect('enter-event', self.fade_in)
		self.connect('leave-event', self.fade_out)

	def insert_songs(self, songs, index):
		model = self.get_property('lomo')
		for song in songs:
			model.insert_uri(Lomo.create_uri(song), index) 

	def fade_in(self, actor, ev):
		self._controls.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0xff,))

	def fade_out(self, actor, ev):
		self._controls.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0x00,))

if __name__ == '__main__':
	Lomo.init(0, "")
	ClutterX11.set_use_argb_visual(True)
	Clutter.init([])
	app = App(sys.argv[1:])
	app.connect('destroy', lambda w: Clutter.main_quit())
	app.show()
	Clutter.main()

