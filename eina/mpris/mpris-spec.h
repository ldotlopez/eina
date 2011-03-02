#ifndef _MPRIS_MPRIS_SPEC_H
#define _MPRIS_MPRIS_SPEC_H

#define MPRIS_BUS_NAME_PREFIX  "org.mpris.MediaPlayer2"
#define MPRIS_OBJECT_PATH     "/org/mpris/MediaPlayer2"
#define MPRIS_ROOT_INTERFACE   "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

const char *mpris_introspection_xml =
	"<node>"
	"  <interface name='" MPRIS_ROOT_INTERFACE "'>"
	"    <method name='Raise'/>"
	"    <method name='Quit'/>"
	"    <property name='CanQuit' type='b' access='read'/>"
	"    <property name='CanRaise' type='b' access='read'/>"
	"    <property name='HasTrackList' type='b' access='read'/>"
	"    <property name='Identity' type='s' access='read'/>"
	"    <property name='DesktopEntry' type='s' access='read'/>"
	"    <property name='SupportedUriSchemes' type='as' access='read'/>"
	"    <property name='SupportedMimeTypes' type='as' access='read'/>"
	"  </interface>"
	"  <interface name='" MPRIS_PLAYER_INTERFACE"'>"
	"    <method name='Next'/>"
	"    <method name='Previous'/>"
	"    <method name='Pause'/>"
	"    <method name='PlayPause'/>"
	"    <method name='Stop'/>"
	"    <method name='Play'/>"
	"    <method name='Seek'>"
	"      <arg direction='in' name='Offset' type='x'/>"
	"    </method>"
	"    <method name='SetPosition'>"
	"      <arg direction='in' name='TrackId' type='o'/>"
	"      <arg direction='in' name='Position' type='x'/>"
	"    </method>"
	"    <method name='OpenUri'>"
	"      <arg direction='in' name='Uri' type='s'/>"
	"    </method>"
	"    <signal name='Seeked'>"
	"      <arg name='Position' type='x'/>"
	"    </signal>"
	"    <property name='PlaybackStatus' type='s' access='read'/>"
	"    <property name='LoopStatus' type='s' access='readwrite'/>"
	"    <property name='Rate' type='d' access='readwrite'/>"
	"    <property name='Shuffle' type='b' access='readwrite'/>"
	"    <property name='Metadata' type='a{sv}' access='read'/>"
	"    <property name='Volume' type='d' access='readwrite'/>"
	"    <property name='Position' type='x' access='read'/>"
	"    <property name='MinimumRate' type='d' access='read'/>"
	"    <property name='MaximumRate' type='d' access='read'/>"
	"    <property name='CanGoNext' type='b' access='read'/>"
	"    <property name='CanGoPrevious' type='b' access='read'/>"
	"    <property name='CanPlay' type='b' access='read'/>"
	"    <property name='CanPause' type='b' access='read'/>"
	"    <property name='CanSeek' type='b' access='read'/>"
	"    <property name='CanControl' type='b' access='read'/>"
	"  </interface>"
	/*
	"  <interface name='org.mpris.MediaPlayer2.TrackList'>"
	"    <method name='GetTracksMetadata'>"
	"      <arg direction='in' name='TrackIds' type='ao'/>"
	"      <arg direction='out' name='Metadata' type='aa{sv}'/>"
	"    </method>"
	"    <method name='AddTrack'>"
	"      <arg direction='in' name='Uri' type='s'/>"
	"      <arg direction='in' name='AfterTrack' type='o'/>"
	"      <arg direction='in' name='SetAsCurrent' type='b'/>"
	"    </method>"
	"    <method name='RemoveTrack'>"
	"      <arg direction='in' name='TrackId' type='o'/>"
	"    </method>"
	"    <method name='GoTo'>"
	"      <arg direction='in' name='TrackId' type='o'/>"
	"    </method>"
	"    <signal name='TrackListReplaced'>"
	"      <arg name='Tracks' type='ao'/>"
	"      <arg name='CurrentTrack' type='o'/>"
	"    </signal>"
	"    <signal name='TrackAdded'>"
	"      <arg name='Metadata' type='a{sv}'/>"
	"      <arg name='AfterTrack' type='o'/>"
	"    </signal>"
	"    <signal name='TrackRemoved'>"
	"      <arg name='TrackId' type='o'/>"
	"    </signal>"
	"    <signal name='TrackMetadataChanged'>"
	"      <arg name='TrackId' type='o'/>"
	"      <arg name='Metadata' type='a{sv}'/>"
	"    </signal>"
	"    <property name='Tracks' type='ao' access='read'/>"
	"    <property name='CanEditTracks' type='b' access='read'/>"
	"  </interface>"
	*/
	"</node>";

#endif
