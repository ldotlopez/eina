#!/usr/bin/python

import os
import sys
import urllib
from gi.repository import Clutter, ClutterX11, Mx, Lomo, GObject, GLib

class Cover(Clutter.Box):
	__gtype_name__ = 'GlosseCover'

	#
	# Member, setter and getter for the 'lomo' property
	#
	def _get_lomo_player_prop(self):
		return self._lomo_player_prop

	def _set_lomo_player_prop(self, lomo_player):
		self._lomo_player_prop = lomo_player
		if not lomo_player:
			raise Exception('No lomo-player')

		lomo_player.connect('notify::current', lambda f, t: self._sync_from_model())
		self._sync_from_model()

	lomo_player = GObject.property (type = Lomo.Player,
		setter = _set_lomo_player_prop, getter = _get_lomo_player_prop,
		flags = GObject.PARAM_READWRITE)

	def __init__(self, *args, **kwargs):
		self._layout = Clutter.BinLayout()
		Clutter.Box.__init__(self, *args, layout_manager = self._layout, **kwargs)
		self.set_property('lomo-player', kwargs['lomo_player'])

		self._actors    = []
		self._timelines = []

		self.set_from_file(os.path.join(os.path.dirname(__file__), 'cover-default.png'))

	def _sync_from_model(self):
		"""
		Sync data from model
		"""
		lomo_player = self.get_property('lomo-player')
		if not lomo_player:
			raise Exception('Missing model')

		stream = lomo_player.get_nth_stream(lomo_player.get_current())
		art    = stream.get_extended_metadata('art-data')

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

		if len(self._actors) > 0:
			t = self._actors[-1]
			t.animatev(Clutter.AnimationMode.LINEAR, 1000,
				("opacity",) , (0x00,))

		self._actors.append(texture)
		self._timelines.append(timeline)

		self._layout.add(texture,
			Clutter.BinAlignment.CENTER, Clutter.BinAlignment.CENTER)
		texture.show()

	def timeline_completed_cb(self, timeline):
		try:
			index = self._timelines.index(timeline)
		except ValueError:
			print "Invalid timeline"
			return

		if index == 0:
			return
		index = index - 1
		actor = self._actors[index]
		self.remove_actor(actor)

		self._actors.pop(index)
		self._timelines.pop(index)

class Seek(Clutter.Box):
	__gtype_name__ = 'GlosseSeek'

	def _get_lomo_player_prop(self):
		return self._lomo_player_prop

	def _set_lomo_player_prop(self, lomo_player):
		if not lomo_player:
			raise Exception('Missing lomo')
		self._lomo_player_prop = lomo_player
		lomo_player.connect('notify', lambda a, b: self._update_from_model())
		self._update_from_model()

	lomo_player = GObject.property(type = Lomo.Player,
		getter = _get_lomo_player_prop, setter = _set_lomo_player_prop,
		flags = GObject.PARAM_READWRITE)

	def __init__(self, *args, **kwargs):
		self._updater_id = 0
		self._inhibitor = False
		self._slider = None

		layout = Clutter.TableLayout()
		super(Seek, self).__init__(*args, layout_manager = layout, **kwargs)

		white = Clutter.Color()
		white.from_string('#ffffffff')

		self._curr = Clutter.Text(text = '1:23', color = white)
		layout.pack(self._curr, 0, 0)

		self._slider = Mx.Slider()
		layout.pack(self._slider, 1, 0)

		self._total = Clutter.Text(text = '5:14', color = white)
		layout.pack(self._total, 2, 0)

		self._slider.connect('notify::value', self._on_notify_value)

	def _on_notify_value(self, widget, prop):
		if self._inhibitor:
			return
		lomo = self.get_property('lomo-player')
		pos  = lomo.get_length() * self._slider.get_value()
		lomo.set_position(pos)

	def _update_from_model(self):
		lomo = self.get_property('lomo-player')
		if not lomo:
			raise Exception('Missing model')

		self._inhibitor = True
		state = lomo.get_state()
		#print "State %s ID: %d" % (repr(Lomo.State.PLAY), self._updater_id )
		if state == Lomo.State.PLAY and (self._updater_id == 0):
			self._updater_id = GLib.timeout_add(500, self._update_from_model_timeout_helper)

		elif state != Lomo.State.PLAY and self._updater_id > 0:
			GLib.source_remove(self._updater_id)
			self._updater_id = 0

		pos = lomo.get_position()
		if pos == -1:
			self._slider.set_value(0)
			self._curr.set_text('-:--')
			self._total.set_text('-:--')
		else:
			print pos
			secs  = pos / 1e9
			total = lomo.get_length() / 1e9
			self._slider.set_value(pos / float(lomo.get_length()))
			self._curr.set_text("%d:%02d" % (secs / 60, secs % 60))
			self._total.set_text("%d:%02d" % (total / 60, total % 60))

		self._inhibitor = False

	def _update_from_model_timeout_helper(self):
		self._update_from_model()
		return True

class Controls(Clutter.Box):
	__gtype_name__ = 'GlosseControls'

	def _set_lomo_player_prop(self, lomo):
		self._lomo_prop = lomo
		if not lomo:
			raise Exception('No lomo-player')

		lomo.connect('notify::state', lambda l,state: self.sync_from_model())
		self.sync_from_model()

	def _get_lomo_player_prop(self):
		return self._lomo_prop

	lomo_player = GObject.property (type = Lomo.Player,
		setter = _set_lomo_player_prop, getter = _get_lomo_player_prop,
		flags = GObject.PARAM_READWRITE)

	def __init__(self, *args, **kwargs):
		layout = Clutter.TableLayout()
		super(Controls, self).__init__(*args, layout_manager = layout, **kwargs)

		d = (('previous', 'media-skip-backward' ),
			 ('playback', 'media-playback-start'),
			 ('next',     'media-skip-forward'  ))

		self._buttons = dict()
		for index, (id_, icon_name) in enumerate(d):
			button = Mx.Button()
			button.add_actor(Mx.Icon(
				icon_name = icon_name,
				icon_size = 32))
			button.show_all()

			layout.pack(button, index, 0)
			self._buttons[id_] =  button
			self._buttons[id_].connect('clicked', self._button_clicked_cb)

		self.set_property('lomo-player', kwargs['lomo_player'])


	def sync_from_model(self):
		if not hasattr(self, '_buttons'):
			return

		lomo = self.get_property('lomo-player')
		if not lomo:
			raise Exception('Missing model')

		state = lomo.get_state()

		if state == Lomo.State.PLAY:
			icon_name = 'media-playback-pause'

		elif state in (Lomo.State.STOP, Lomo.State.PAUSE):
			icon_name = 'media-playback-start'

		else:
			raise Exception('Unknow state')

		self._buttons['playback'].set_icon_name(icon_name)

	def _button_clicked_cb(self, w):
		lomo = self.get_property('lomo-player')
		if lomo is None:
			raise Exception('No lomo')

		if w == self._buttons['previous']:
			i = lomo.get_previous()
			if i < 0:
				return
			lomo.set_current(i)

		elif w == self._buttons['next']:
			i = lomo.get_next()
			if i < 0:
				return
			lomo.set_current(i)

		else:
			lomo.toggle_playback_state()

class App(Clutter.Stage):
	__gtype_name__ = 'GlosseApp'

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

		self.set_property('lomo', Lomo.Player(random = True, repeat = True))
		self.insert_songs(uris, 0)

		bg_color = Clutter.Color()
		bg_color.from_string('#000000ff')
		self.set_color(bg_color)

		# Setup main container
		main_layout = Clutter.BinLayout()
		main_box    = Clutter.Box(layout_manager = main_layout)
		main_box.add_constraint(Clutter.BindConstraint.new(self, Clutter.BindCoordinate.SIZE, 0.0))
		main_box.show()
		self.add_actor(main_box)

		# Setup cover (or background)
		self._cover = Cover(lomo_player = self._get_lomo())
		self._cover.show()
		main_layout.add(self._cover, Clutter.BinAlignment.FILL, Clutter.BinAlignment.FILL)

		bottom_layout = Clutter.TableLayout()
		self._bottom_box = Clutter.Box(opacity = 0x00, layout_manager = bottom_layout)

		# Setup controls
		self._controls = Controls(lomo_player = self._get_lomo())
		bottom_layout.pack(self._controls, 0, 0)

		# Setup seek
		self._seek = Seek(lomo_player = self._get_lomo())
		bottom_layout.pack(self._seek, 0, 1)

		# Add bottom_box
		main_layout.add(self._bottom_box, Clutter.BinAlignment.CENTER, Clutter.BinAlignment.END)

		self.connect('enter-event', self.fade_in)
		self.connect('leave-event', self.fade_out)

	def insert_songs(self, songs, index):
		model = self.get_property('lomo')
		for song in songs:
			model.insert_uri(Lomo.create_uri(song), index)

	def fade_in(self, actor, ev):
		self._bottom_box.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0xff,))

	def fade_out(self, actor, ev):
		self._bottom_box.animatev(Clutter.AnimationMode.EASE_OUT_EXPO, 500,
			("opacity",), (0x00,))

if __name__ == '__main__':
	Lomo.init(0, "")
	ClutterX11.set_use_argb_visual(True)
	Clutter.init([])
	app = App(sys.argv[1:])
	app.connect('destroy', lambda w: Clutter.main_quit())
	app.show()
	Clutter.main()

