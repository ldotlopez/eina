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
AM_INIT_AUTOMAKE([1.10 -Wno-portability foreign])
IT_PROG_INTLTOOL([0.40.0])
LT_INIT
AM_GLIB_GNU_GETTEXT
AM_PROG_VALAC([0.10.0])
GTK_DOC_CHECK(1.9)

#
# Package information
#
GETTEXT_PACKAGE="eina"
EINA_API_VERSION="0.10"
EINA_VERSION="0.10.0"
EINA_CODENAME="La primavera"              # Delafe y las flores azules
EINA_CODENAME_0_9_4="Trained"             # Najwa
EINA_CODENAME_0_9_3="White rabbit"        # Jefferson airplane
EINA_CODENAME_0_9_2="Pagan poetry"        # Björk
EINA_CODENAME_0_9_1="In muine we believe" #
EINA_CODENAME_0_9_0="They don't believe"  # Russian Red

GEL_MAJOR=1
GEL_MINOR=1
GEL_MICRO=1
GEL_API_VERSION="$GEL_MAJOR.$GEL_MINOR"
GEL_VERSION="$GEL_API_VERSION.$GEL_MICRO"
AC_SUBST(GEL_API_VERSION)
AC_SUBST(GEL_VERSION)
AC_SUBST(GEL_MAJOR)
AC_SUBST(GEL_MINOR)
AC_SUBST(GEL_MICRO)

LOMO_MAJOR=1
LOMO_MINOR=1
LOMO_MICRO=1
LOMO_API_VERSION="$LOMO_MAJOR.$LOMO_MINOR"
LOMO_VERSION="$LOMO_API_VERSION.$LOMO_MICRO"
AC_SUBST(LOMO_API_VERSION)
AC_SUBST(LOMO_VERSION)
AC_SUBST(LOMO_MAJOR)
AC_SUBST(LOMO_MINOR)
AC_SUBST(LOMO_MICRO)

AC_DEFINE_UNQUOTED(EINA_CODENAME,"$EINA_CODENAME", [Eina version codename])
AC_DEFINE_UNQUOTED(EINA_API_VERSION,"$EINA_API_VERSION", [Eina API version])

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
AC_CONFIG_MACRO_DIR([m4])
# ACLOCAL_AMFLAGS="-I build/m4 ${ACLOCAL_FLAGS}"
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl pkgconfig
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$top_builddir/data"

# Introspection, disable for now
GOBJECT_INTROSPECTION_CHECK([0.6.7])

#
# checks for programs
#
AC_PROG_CC
# AC_PROG_CXX
AC_PATH_PROG(PKG_CONFIG, pkg-config)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource)
AC_PATH_PROG(GLIB_COMPILE_SCHEMAS, glib-compile-schemas)
AC_PATH_PROG(VALAC, valac)
#AC_PATH_PROG(VALA_GEN_INTROSPECT, vala-gen-introspect)
#AC_PATH_PROG(VAPIGEN, vapigen)
#AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`

AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(GLIB_COMPILE_SCHEMAS)
AC_SUBST(GDK_PIXBUF_CSOURCE)
AC_SUBST(VALAC)
#AC_SUBST(VALA_GEN_INTROSPECT)
#AC_SUBST(VAPIGEN)
#AC_SUBST(DBUS_BINDING_TOOL)

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
	DEBUG_CFLAGS="-ggdb -O0"
fi
AC_SUBST(DEBUG_CFLAGS)

if test x$enable_experimental = 'xyes'; then
	EXPERIMENTAL_CFLAGS="-DENABLE_EXPERIMENTAL=1"
	HAVE_EXPERIMENTAL=yes
else
	EXPERIMENTAL_CFLAGS="-DENABLE_EXPERIMENTAL=0"
	HAVE_EXPERIMENTAL=no
fi
AM_CONDITIONAL(BUILD_EXPERIMENTAL, test x"$HAVE_EXPERIMENTAL" = "xyes")

dnl
dnl core configuration
dnl

COMMON_CFLAGS="\
	$TOPAZ_CFLAGS \
	-DGEL_DISABLE_DEPRECATED -DGEL_IO_DISABLE_DEPRECATED -DGEL_UI_DISABLE_DEPRECATED \
	-Wall -Werror -std=c99 $DEBUG_CFLAGS \
	"

GLIB_API_VERSION="2.0"
GTK_API_VERSION="3.0"
GST_API_VERSION="0.10"

GLIB_VERSION="2.28.0"
GTK_VERSION="3.0.0"
GST_VERSION="$GST_API_VERSION.0"
glib_modules="\
	glib-$GLIB_API_VERSION >= $GLIB_VERSION,    \
	gobject-$GLIB_API_VERSION >= $GLIB_VERSION, \
	gmodule-$GLIB_API_VERSION >= $GLIB_VERSION, \
	gio-$GLIB_API_VERSION >= $GLIB_VERSION      \
	"
gtk_modules="\
	gtk+-$GTK_API_VERSION >= $GTK_VERSION \
	"
gst_modules="\
	gstreamer-0.10 >= $GST_VERSION \
	"
PKG_CHECK_MODULES(GLIB, $glib_modules)
PKG_CHECK_MODULES(GTK,  $gtk_modules)
PKG_CHECK_MODULES(GST,  $gst_modules)

GDK_QUARTZ_BACKEND="`pkg-config --variable=target gdk-3.0`"
AC_DEFINE([GDK_QUARTZ_BACKEND], [1], [Define if GDK is using the quartz backend])

AC_SUBST(GLIB_API_VERSION)
AC_SUBST(GTK_API_VERSION)
AC_SUBST(GST_API_VERSION)

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
	$GLIB_CFLAGS         \
	$GTK_CFLAGS          \
	$GST_CFLAGS          \
	$CURL_CFLAGS         \
	$COMMON_CFLAGS       \
	$EXPERIMENTAL_CFLAGS \
	"
EINA_LIBS="\
	$GLIB_LIBS      \
	$GTK_LIBS       \
	$GST_LIBS       \
	$CURL_LIBS      \
	$COMMON_LIBS    \
	$EXPERIMENTAL_LIBS  \
	"
GLIB_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

GTK_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GST_CFLAGS+=" $COMMON_CFLAGS"
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

# EINA_LIBS="--as-needed $EINA_LIBS"

AC_SUBST(GTK_VERSION)
AC_SUBST(GST_VERSION)

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
CLUTTER_GTK_VERSION="1.0.0"
clutter_modules="clutter-gtk-1.0 >= $CLUTTER_GTK_VERSION"
PKG_CHECK_MODULES(CLUTTER_GTK, $clutter_modules,
[
	if test x"`$PKG_CONFIG --variable=flavour clutter-gtk-0.10`" = "xosx"; then
		echo "*** clutter-gtk is not supported on osx (disabling clutter) ***"
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_CLUTTER_GTK=0"
		HAVE_CLUTTER_GTK=no
	else
		AC_SUBST(CLUTTER_GTK_CFLAGS)
		AC_SUBST(CLUTTER_GTK_LIBS)
		HAVE_CLUTTER_GTK=yes
		EINA_CFLAGS="$EINA_CFLAGS $CLUTTER_GTK_CFLAGS -DHAVE_CLUTTER_GTK=1"
		EINA_LIBS="$EINA_LIBS $CLUTTER_GTK_LIBS"
	fi
],
[
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_CLUTTER_GTK=0"
	HAVE_CLUTTER_GTK=no
])
AM_CONDITIONAL(USE_CLUTTER_GTK, test x"$HAVE_CLUTTER_GTK" = "xyes")

INDICATE_VERSION="0.4.0"
indicate_modules="indicate >= $INDICATE_VERSION"
PKG_CHECK_MODULES(INDICATE, $indicate_modules,
[
	AC_SUBST(INDICATE_CFLAGS)
	AC_SUBST(INDICATE_LIBS)
	HAVE_INDICATE=yes
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_INDICATE=1"
],
[
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_INDICATE=0"
	HAVE_INDICATE=no
])

dnl
dnl notify (ntfy)
dnl
NOTIFY_VERSION="0.7.0"
notify_modules="libnotify >= $NOTIFY_VERSION"
PKG_CHECK_MODULES(NOTIFY, $notify_modules,
[
	AC_SUBST(NOTIFY_CFLAGS)
	AC_SUBST(NOTIFY_LIBS)
	HAVE_NOTIFY=yes
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_NOTIFY=1"
],
[
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_NOTIFY=0"
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
#DBUS_GLIB_VERSION="0.80"
#dbus_glib_modules="dbus-glib-1 >= $DBUS_GLIB_VERSION"
#PKG_CHECK_MODULES(DBUS_GLIB, $dbus_glib_modules,
#	[
#		AC_SUBST(DBUS_GLIB_CFLAGS)
#		AC_SUBST(DBUS_GLIB_LIBS)
#		HAVE_DBUS_GLIB=yes
#		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_DBUS_GLIB=1"
#	],
#	[
#		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_DBUS_GLIB=0"
#		HAVE_DBUS_GLIB=no
#	]
#	)

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
#UUID_VERSION="1.41.0"
#uuid_modules="uuid >= $UUID_VERSION" 
#PKG_CHECK_MODULES(UUID, $uuid_modules,
#	[
#		HAVE_UUID=yes
#	],
#	[
#		dnl On MacOS X uuid is installed by default but is unreachable by
#		dnl pkg-config. We need to check if uuid/uuid.h is present and asume some things
#		AC_CHECK_HEADER(uuid/uuid.h, [HAVE_UUID=yes], [HAVE_UUID=no])
#	]
#	)
#
#if test x"$HAVE_UUID" = "xyes"; then
#	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_UUID=1"
#	AC_SUBST(UUID_CFLAGS)
#	AC_SUBST(UUID_LIBS)
#else
#	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_UUID=0"
#fi

dnl
dnl Conditionals
dnl
AM_CONDITIONAL(BUILD_ADB,      test x"$HAVE_SQLITE3"     = "xyes")
AM_CONDITIONAL(BUILD_CALLHOME, test x"$HAVE_UUID"        = "xyes")
AM_CONDITIONAL(BUILD_CLUTTY,   test x"$HAVE_CLUTTER_GTK" = "xyes")
#AM_CONDITIONAL(BUILD_DBUS,     test x"$HAVE_DBUS_GLIB"   = "xyes" -a x"$DBUS_BINDING_TOOL" != "x")
AM_CONDITIONAL(BUILD_IGE,      test x"$HAVE_IGE"         = "xyes")
AM_CONDITIONAL(BUILD_INDICATE, test x"$HAVE_INDICATE"    = "xyes")
AM_CONDITIONAL(BUILD_NTFY,     test x"$HAVE_NOTIFY"      = "xyes")

dnl Keep in alphabetical order
AC_CONFIG_FILES([
build/Makefile
bundle/build
bundle/Info.plist
data/Makefile
data/eina.desktop
data/gel-${GEL_API_VERSION}.pc
data/gel-${GEL_API_VERSION}-uninstalled.pc
data/gel-ui-${GEL_API_VERSION}.pc
data/gel-ui-${GEL_API_VERSION}-uninstalled.pc
data/lomo-${LOMO_API_VERSION}.pc
data/lomo-${LOMO_API_VERSION}-uninstalled.pc
data/eina-${EINA_API_VERSION}.pc
data/net.sourceforge.eina.gschema.xml.in
docs/Makefile
docs/gel/Makefile
docs/eina/Makefile
docs/lomo/Makefile
icons/Makefile
icons/hicolor/Makefile
gel/Makefile
eina/Makefile
eina/ext/Makefile
eina/adb/Makefile
eina/art/Makefile
eina/clutty/Makefile
eina/dbus/Makefile
eina/dock/Makefile
eina/fieshta/Makefile
eina/lastfm/Makefile
eina/lomo/Makefile
eina/mpris/Makefile
eina/muine/Makefile
eina/ntfy/Makefile
eina/player/Makefile
eina/playlist/Makefile
eina/preferences/Makefile
eina/statusicon/Makefile
Makefile
lomo/Makefile
pixmaps/Makefile
plugins/lastfm/net.sourceforge.eina.plugins.lastfm.gschema.xml.in
po/Makefile.in
])
AC_OUTPUT

# eina/recently/Makefile
