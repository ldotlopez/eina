#
# Init package
#
AC_INIT([Eina],[0.10.0],[https://bugs.launchpad.net/eina],[eina],[http://eina.sourceforge.net/])
AC_CONFIG_HEADERS([config.h])

#
# Prerequisites and some initilizations
#
AC_PREREQ(2.63)
LT_PREREQ([2.2.6])
AM_INIT_AUTOMAKE([1.10])
IT_PROG_INTLTOOL([0.40.0])
LT_INIT
AM_GLIB_GNU_GETTEXT
GTK_DOC_CHECK(1.9)
GOBJECT_INTROSPECTION_CHECK([0.6.7])

#
# Package information
#
GETTEXT_PACKAGE="eina"
EINA_API_VERSION="0.10"
EINA_VERSION="$EINA_API_VERSION.0"
EINA_CODENAME="Trained"                   # Najwa
EINA_CODENAME_0_9_3="White rabbit"        # Jefferson airplane
EINA_CODENAME_0_9_2="Pagan poetry"        # Björk
EINA_CODENAME_0_9_1="In muine we believe" #
EINA_CODENAME_0_9_0="They don't believe"  # Russian Red

AC_DEFINE_UNQUOTED(EINA_CODENAME,"$EINA_CODENAME", [Eina version codename])

AC_SUBST(EINA_VERSION)
AC_SUBST(EINA_CODENAME)
AC_SUBST(EINA_API_VERSION)

PACKAGE_RELEASE_DATE="`date '+%a, %d %b %Y %H:%m:%S %z'`"
AC_SUBST(PACKAGE_RELEASE_DATE)

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

EINA_DOMAIN="net.sourceforge.eina"
AC_SUBST(EINA_DOMAIN)
AC_DEFINE_UNQUOTED(EINA_DOMAIN,"$EINA_DOMAIN", [Eina domain])

# for po/
DATADIRNAME="share"
AC_SUBST(DATADIRNAME)

dnl aclocal flags
ACLOCAL_AMFLAGS="-I m4 ${ACLOCAL_FLAGS}"

#
# checks for programs
#
AC_PROG_CC
# AC_PROG_CXX
AC_PATH_PROG(PKG_CONFIG, pkg-config)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource)
AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`

AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(DBUS_BINDING_TOOL)
AC_SUBST(GDK_PIXBUF_CSOURCE)
#
# checks for libraries
#
AC_SEARCH_LIBS([strerror],[cposix])

#
# checks for header files
#
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h])

# Detect OSX
#AC_LANG_PUSH([Objective C])
#OLD_AM_CFLAGS="$AM_CFLAGS"
#AM_CFLAGS="$AM_CFLAGS -framework Cocoa"
#AC_CHECK_HEADERS([/System/Library/Frameworks/Cocoa.framework/Headers/Cocoa.h], [HAVE_COCOA_H="yes"], [HAVE_COCOA_H="no"])
#AM_CFLAGS="$OLD_AM_CFLAGS"
#AC_LANG_POP([Objective C])

#
# checks for types
#
AC_TYPE_SIZE_T

#
# checks for structures
#

#
# checks for compiler characteristics
#
AC_C_INLINE
AM_PROG_CC_C_O

#
# checks for library functions
#
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])

#
# checks for system services
#

AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# export PKG_CONFIG_PATH=/Library/Frameworks/Gtk.framework/Resources/dev/lib/pkgconfig:\
# /Library/Frameworks/Cairo.framework/Resources/dev/lib/pkgconfig:\
# /Library/Frameworks/GLib.framework/Resources/dev/lib/pkgconfig 


#*******************************************************************************
# Internationalization
#*******************************************************************************
# Some flags
TOPAZ_CFLAGS="-DATK_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED \
	-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED \
	-DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE -DGSEAL_ENABLE"

AC_ARG_ENABLE(release-build, [  --enable-release-build      Build in release mode (-debug,+optimizations)], release_build=yes, release_build=no)
AC_ARG_ENABLE(experimental,  [  --enable-experimental       Compile with experimental features], enable_experimental=yes, enable_experimental=no)

if test x$release_build = 'xyes'; then
	DEBUG_CFLAGS="-O2"
else
	DEBUG_CFLAGS="-g -O0"
fi

if test x$enable_experimental = 'xyes'; then
	EXPERIMENTAL_CFLAGS="-DENABLE_EXPERIMENTAL=1"
	HAVE_EXPERIMENTAL=yes
else
	EXPERIMENTAL_CFLAGS="-DENABLE_EXPERIMENTAL=0"
	HAVE_EXPERIMENTAL=no
fi
AM_CONDITIONAL(BUILD_EXPERIMENTAL, test x"$HAVE_EXPERIMENTAL" = "xyes")

enable_plugins='yes'
if test x$enable_plugins = 'xyes'; then
	HAVE_PLUGINS=yes
else
	HAVE_PLUGINS=no
fi
AM_CONDITIONAL(BUILD_PLUGINS, test x"$HAVE_PLUGINS" = "xyes")

dnl
dnl core configuration
dnl

COMMON_CFLAGS="\
	$TOPAZ_CFLAGS \
	-DGEL_DISABLE_DEPRECATED -DGEL_IO_DISABLE_DEPRECATED -DGEL_UI_DISABLE_DEPRECATED \
	-Wall -Werror -std=c99 $BUILD_MODE_CFLAGS \
	"

GLIB_VERSION="2.25.9"
GTK_VERSION="2.90"
GSTREAMER_VERSION="0.10"
glib_modules="\
	glib-2.0 >= $GLIB_VERSION,    \
	gmodule-2.0 >= $GLIB_VERSION, \
	gio-2.0 >= $GLIB_VERSION      \
	"
gtk_modules="\
	gtk+-3.0 >= $GTK_VERSION \
	"
gstreamer_modules="\
	gstreamer-0.10 >= $GSTREAMER_VERSION \
	"

PKG_CHECK_MODULES(GLIB,      $glib_modules)
PKG_CHECK_MODULES(GTK,       $gtk_modules)
PKG_CHECK_MODULES(GSTREAMER, $gstreamer_modules)

GDK_QUARTZ_BACKEND="`pkg-config --variable=target gdk-3.0`"
AC_DEFINE([GDK_QUARTZ_BACKEND], [1], [Define if GDK is using the quartz backend])

AC_PATH_PROG(CURL_CONFIG, curl-config)
if test x$CURL_CONFIG = 'x'; then
	echo "curl-config not found"
	exit 1
fi

CURL_CFLAGS="`$CURL_CONFIG --cflags` $COMMON_CFLAGS"
CURL_LIBS="`$CURL_CONFIG --libs`"
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

EINA_CFLAGS="\
	$GLIB_CFLAGS        \
	$GTK_CFLAGS         \
	$GSTREAMER_CFLAGS   \
	$CURL_CFLAGS        \
	$COMMON_CFLAGS      \
	$DEBUG_CFLAGS       \
	$EXPERIMENTA_CFLAGS \
	"
EINA_LIBS="\
	$GLIB_LIBS      \
	$GTK_LIBS       \
	$GSTREAMER_LIBS \
	$CURL_LIBS      \
	$COMMON_LIBS    \
	$DEBUG_LIBS     \
	$EXPERIMENTAL_LIBS  \
	"
GLIB_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

GTK_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GSTREAMER_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

# EINA_LIBS="--as-needed $EINA_LIBS"

AC_SUBST(GTK_VERSION)
AC_SUBST(GSTREAMER_VERSION)

GLIB_GSETTINGS

dnl
dnl Optional deps: clutter, notify, ige-mac-integration, dbus-glib, sqlite3, unique, uuid
dnl

AC_SUBST(EINA_LIBS)
AC_SUBST(EINA_CFLAGS)

dnl
dnl =====================================================================
dnl =====================================================================
dnl

dnl
dnl Clutter (core, fieshta)
dnl
CLUTTER_GTK_VERSION="9.10.0"
clutter_modules="clutter-gtk-0.10 >= $CLUTTER_GTK_VERSION"
PKG_CHECK_MODULES(CLUTTER_GTK, $clutter_modules,
[
	if test x"`$PKG_CONFIG --variable=flavour clutter-gtk-0.10`" = "xosx"; then
		echo "*** clutter-gtk is not supported on osx (disabling clutter) ***"
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_CLUTTER_GTK=0"
		HAVE_CLUTTER_GTK=no
	else
		AC_SUBST(CLUTTER_GTK_CFLAGS)
		AC_SUBST(CLUTTER_GTK_LIBS)
		HAVE_CLUTTER_GTK=yes
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS $CLUTTER_GTK_CFLAGS DHAVE_CLUTTER_GTK=1"
		PACKAGE_LIBS="$PACKAGE_LIBS $CLUTTER_GTK_LIBS"
	fi
],
[
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_CLUTTER_GTK=0"
	HAVE_CLUTTER_GTK=no
])
AM_CONDITIONAL(USE_CLUTTER_GTK, test x"$HAVE_CLUTTER_GTK" = "xyes")

dnl
dnl notify (ntfy)
dnl
NOTIFY_VERSION="9.4.0"
notify_modules="libnotify >= $NOTIFY_VERSION"
PKG_CHECK_MODULES(NOTIFY, $notify_modules,
[
	AC_SUBST(NOTIFY_CFLAGS)
	AC_SUBST(NOTIFY_LIBS)
	HAVE_NOTIFY=yes
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_NOTIFY=1"
],
[
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_NOTIFY=0"
	HAVE_NOTIFY=no
])

dnl
dnl ige-mac-integracion (ige)
dnl
IGE_VERSION="0.9.0"
ige_modules="ige-mac-integration >= $IGE_VERSION"
if test "x`pkg-config --variable target gtk+-3.0`" = "xquartz"; then
	PKG_CHECK_MODULES(IGE, $ige_modules,
		[
			HAVE_IGE=yes
			EINA_CFLAGS="$EINA_CFLAGS -DHAVE_IGE=1"
			AC_SUBST(IGE_CFLAGS)
			AC_SUBST(IGE_LIBS)
		],
		[
			EINA_CFLAGS="$EINA_CFLAGS -DHAVE_IGE=0"
			HAVE_IGE=no
		]
		)
else
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_IGE=0"
	HAVE_IGE=no
fi

dnl
dnl dbus-glib (dbus)
dnl
DBUS_GLIB_VERSION="0.80"
dbus_glib_modules="dbus-glib-1 >= $DBUS_GLIB_VERSION"
PKG_CHECK_MODULES(DBUS_GLIB, $dbus_glib_modules,
	[
		AC_SUBST(DBUS_GLIB_CFLAGS)
		AC_SUBST(DBUS_GLIB_LIBS)
		HAVE_DBUS_GLIB=yes
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_DBUS_GLIB=1"
	],
	[
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_DBUS_GLIB=0"
		HAVE_DBUS_GLIB=no
	]
	)

dnl
dnl sqlite3 (!core, adb)
dnl
SQLITE3_VERSION="3.2.0"
sqlite3_modules="sqlite3 >= $SQLITE3_VERSION"
PKG_CHECK_MODULES(SQLITE3, $sqlite3_modules,
	[
		HAVE_SQLITE3=yes
	],
	[
		dnl On MacOS X sqlite3 is installed by default but is unreachable by
		dnl pkg-config. We need to check if sqlite3.h is present and asume some things
		AC_CHECK_HEADER(sqlite3.h, [HAVE_SQLITE3=yes], [HAVE_SQLITE3=no])
		if test x"$HAVE_SQLITE3" = "xyes"; then
			SQLITE3_LIBS="-lsqlite3"
		fi
	]
	)
if test x"$HAVE_SQLITE3" = "xyes"; then
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_SQLITE3=1"
	AC_SUBST(SQLITE3_CFLAGS)
	AC_SUBST(SQLITE3_LIBS)
else
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_SQLITE3=0"
fi


dnl
dnl uuid (!core, callhome)
dnl
UUID_VERSION="1.41.0"
uuid_modules="uuid >= $UUID_VERSION" 
PKG_CHECK_MODULES(UUID, $uuid_modules,
	[
		HAVE_UUID=yes
	],
	[
		dnl On MacOS X uuid is installed by default but is unreachable by
		dnl pkg-config. We need to check if uuid/uuid.h is present and asume some things
		AC_CHECK_HEADER(uuid/uuid.h, [HAVE_UUID=yes], [HAVE_UUID=no])
	]
	)

if test x"$HAVE_UUID" = "xyes"; then
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_UUID=1"
	AC_SUBST(UUID_CFLAGS)
	AC_SUBST(UUID_LIBS)
else
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_UUID=0"
fi

dnl
dnl Conditionals
dnl
AM_CONDITIONAL(GEL_UI_ENABLE_APPLICATION, test "1" = "0")
AM_CONDITIONAL(BUILD_ADB,      test x"$HAVE_SQLITE3"     = "xyes")
AM_CONDITIONAL(BUILD_CALLHOME, test x"$HAVE_UUID"        = "xyes")
AM_CONDITIONAL(BUILD_CLUTTY,   test x"$HAVE_CLUTTER_GTK" = "xyes")
AM_CONDITIONAL(BUILD_DBUS,     test x"$HAVE_DBUS_GLIB"   = "xyes" -a x"$DBUS_BINDING_TOOL" != "x")
AM_CONDITIONAL(BUILD_IGE,      test x"$HAVE_IGE"         = "xyes")
AM_CONDITIONAL(BUILD_NTFY,     test 0 -eq 1)

dnl Keep in alphabetical order
AC_CONFIG_FILES([
build/Makefile
bundle/build
bundle/Info.plist
data/eina.desktop
data/eina-0.10.pc
data/net.sourceforge.eina.gschema.xml.in
gel/Makefile
lomo/Makefile
eina/Makefile
eina/ext/Makefile
eina/application/Makefile
eina/art/Makefile
eina/dock/Makefile
eina/lomo/Makefile
eina/player/Makefile
eina/playlist/Makefile
eina/adb/Makefile
eina/muine/Makefile
eina/recently/Makefile
pixmaps/Makefile
data/Makefile
Makefile
docs/Makefile
icons/Makefile
icons/hicolor/Makefile
po/Makefile.in
plugins/lastfm/net.sourceforge.eina.plugins.lastfm.gschema.xml.in
])
AC_OUTPUT

