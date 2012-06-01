#coding: utf-8

#Copyright (C) 2011 Jacobo Tarragón
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
This plug-in adds a new tab to the Eina dock
that shows the current stream lyrics,
retrieved from lyrics.wikia.org

TO DO:
    * Cache lyrics
    * Dock icon
"""

import re
import urllib, urlparse
from string import capwords
from threading import Thread

from gi.repository import Gtk, Eina, GObject


WIDGET_LABEL = u"Lyrics"
MSG_NOT_FOUND = u"\nNo lyrics found for \"%(title)s\" by %(artist)s"
MSG_LOADING = u"\nsearching lyrics…"
MSG_NO_TAGS = u"\nwaiting for the stream tags to be loaded…"


class LyricsDownloader(Thread):
    """Used to fetch the lyrics from the artist and song title"""

    def __init__(self, plugin):
        super(LyricsDownloader, self).__init__()
        self.plugin = plugin

        self.base_url = "http://lyrics.wikia.org"
        self.lyrics_re = re.compile("</div>(&#.*)<!--")

    def _get_url(self, artist, title):
        # q = lambda str: urllib.quote(capwords(str))
        return fixurl("%s/%s:%s" % (self.base_url, capwords(artist), capwords(title)))

    def _unescape(self, text):
        def fixup(m):
            text = m.group(0)
            if text[:2] == "&#":
                # unicode character reference
                try:
                    if text[2] == "x":
                        return unichr(int(text[3:-1], 16))
                    else:
                        return unichr(int(text[2:-1]))
                except ValueError:
                    pass
            else:
                # named entity
                try:
                    text = unichr(htmlentitydefs.name2codepoint[text[1:-1]])
                except KeyError:
                    pass
            return text # leave as is
        return re.sub("&#?\w+;", fixup, text)

    def get_lyrics(self, artist, title):
        self.artist = artist
        self.title = title
        self.start()

    def run(self):
        lyrics = self._fetch_lyrics()
        self.plugin._display_lyrics(lyrics)

    def _fetch_lyrics(self):
        if not self.artist or not self.title:
            return None
        url = self._get_url(self.artist, self.title)
        src = urllib.urlopen(url).read()
        enc_lyrics = self.lyrics_re.search(src)
        lyrics = None
        if enc_lyrics:
            try:
                lyrics = self._unescape(enc_lyrics.groups()[0])
                lyrics = lyrics.replace('<br />', '\n')
                lyrics = '\n' + lyrics + '\n'
            except IndexError:
                pass
        return lyrics


class LyricsPlugin(GObject.Object, Eina.Activatable):
    """Creates a new dock tab which shows the lyrics for the current stream"""

    __gtype_name__ = 'EinaLyricsPlugin'
    application = GObject.property(type = Eina.Application)

    def __init__(self, *args, **kwargs):
        super(LyricsPlugin, self).__init__(*args, **kwargs)
        self.label = WIDGET_LABEL
        self.textview = None
        self.textbuffer = None
        self.widget = None
        self.current_stream = None
        self.lyrics_downloader = None

    def do_activate(self, app):
        self.app = app
        self.lomo = app.get_lomo()
        self.create_widget()
        self.lomo.connect('change', self.update_stream_lyrics)
        return True

    def do_deactivate(self, app):
        dock = app.get_dock()
        dock.remove_widget(self.widget)
        self.lomo.disconnect_by_func(self.update_stream_lyrics)
        self.textview = None
        self.textbuffer = None
        self.widget = None
        return True

    def create_widget(self):
        self.textview = Gtk.TextView()
        self.textview.set_editable(False)
        self.textview.set_cursor_visible(False)
        self.textview.set_justification(Gtk.Justification.CENTER)
        self.textview.set_wrap_mode(Gtk.WrapMode.WORD)
        self.textbuffer = self.textview.get_buffer()
        self.textview.show()

        self.widget = Gtk.ScrolledWindow()
        self.widget.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.widget.add(self.textview)
        self.app.add_dock_widget('lyrics_plugin', self.widget, Gtk.Label(self.label), 0)
        self.widget.show_all()

    def _stream_tags_ready(self, lomo, stream):
        if stream == self.current_stream:
            self.lomo.disconnect_by_func(self._stream_tags_ready)
            self.update_stream_lyrics(self.lomo)

    def _clear_lyrics(self, clear_text='', not_found=False):
        if not_found:
            artist = self.current_stream.get_tag('artist').decode('utf-8')
            title = self.current_stream.get_tag('title').decode('utf-8')
            self.textbuffer.set_text(MSG_NOT_FOUND % {'artist': artist, 'title': title})
        elif not self.current_stream:
            self.textbuffer.set_text(clear_text)

        return True

    def _display_lyrics(self, lyrics):
        if lyrics:
            self.textbuffer.set_text(lyrics)
        else:
            return self._clear_lyrics(not_found=True)

    def update_stream_lyrics(self, lomo, from_stream=None, to_stream=None):
        self.lyrics_downloader = LyricsDownloader(self)
        self.textbuffer.set_text(MSG_LOADING)
        if to_stream is None:
            to_stream = self.lomo.get_current()
        self.current_stream = self.lomo.get_nth_stream(to_stream)
        if not self.current_stream.get_all_tags_flag():
            self.lomo.connect('all_tags', self._stream_tags_ready)
            return self._clear_lyrics(MSG_NO_TAGS)
        else:
            artist = self.current_stream.get_tag('artist').decode('utf-8')
            title = self.current_stream.get_tag('title').decode('utf-8')
            self.lyrics_downloader.get_lyrics(artist, title)

        return True


def fixurl(url):
    #http://stackoverflow.com/a/804380
    # turn string into unicode
    if not isinstance(url,unicode):
        url = url.decode('utf8')

    # parse it
    parsed = urlparse.urlsplit(url)

    # divide the netloc further
    userpass,at,hostport = parsed.netloc.rpartition('@')
    user,colon1,pass_ = userpass.partition(':')
    host,colon2,port = hostport.partition(':')

    # encode each component
    scheme = parsed.scheme.encode('utf8')
    user = urllib.quote(user.encode('utf8'))
    colon1 = colon1.encode('utf8')
    pass_ = urllib.quote(pass_.encode('utf8'))
    at = at.encode('utf8')
    host = host.encode('idna')
    colon2 = colon2.encode('utf8')
    port = port.encode('utf8')
    path = '/'.join(  # could be encoded slashes!
        urllib.quote(urllib.unquote(pce).encode('utf8'),'')
        for pce in parsed.path.split('/')
    )
    query = urllib.quote(urllib.unquote(parsed.query).encode('utf8'),'=&?/')
    fragment = urllib.quote(urllib.unquote(parsed.fragment).encode('utf8'))

    # put it back together
    netloc = ''.join((user,colon1,pass_,at,host,colon2,port))
    return urlparse.urlunsplit((scheme,netloc,path,query,fragment))
