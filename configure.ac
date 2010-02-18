#
# Init package
#
AC_INIT([Eina],[0.9.3],[https://bugs.launchpad.net/eina],[eina],[http://eina.sourceforge.net/])
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
#
# Package information
#
GETTEXT_PACKAGE="eina"
EINA_API_VERSION="0.9"
EINA_VERSION="$EINA_API_VERSION.3"
EINA_CODENAME="White rabbit"              # Jefferson airplane
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
AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(DBUS_BINDING_TOOL)

#
# checks for libraries
#
AC_SEARCH_LIBS([strerror],[cposix])

#
# checks for header files
#
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h])

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
	-DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"

AC_ARG_ENABLE(release-build, [  --enable-release-build      Build in release mode (-debug,+optimizations)], release_build=yes, release_build=no)
AC_ARG_ENABLE(experimental,  [  --enable-experimental       Compile with experimental features], enable_experimental=yes, enable_experimental=no)
AC_ARG_ENABLE(clutty,        [  --enable-clutty             Use clutty cover     ], enable_clutty=yes, enable_clutty=no)

if test x$release_build = 'xyes'; then
	CXXFLAGS="-O2"
	CFLAGS="-O2"
	BUILD_MODE_CFLAGS="-DG_DISABLE_CAST_CHECKS -O2"
else
	CXXFLAGS="-O0"
	CFLAGS="-O0"
	BUILD_MODE_CFLAGS="-g -O0"
fi

if test x$enable_experimental = 'xyes'; then
	CXXFLAGS="$CXXFLAGS -DENABLE_EXPERIMENTAL=1"
	CFLAGS="$CFLAGS -DENABLE_EXPERIMENTAL=1"
	HAVE_EXPERIMENTAL=yes
else
	CXXFLAGS="$CXXFLAGS -DENABLE_EXPERIMENTAL=0"
	CFLAGS="$CFLAGS -DENABLE_EXPERIMENTAL=0"
	HAVE_EXPERIMENTAL=no
fi
AM_CONDITIONAL(BUILD_EXPERIMENTAL, test x"$HAVE_EXPERIMENTAL" = "xyes")

dnl
dnl core configuration
dnl

GLIB_VERSION="2.16"
GTK_VERSION="2.14"
GSTREAMER_VERSION="0.10"
pkg_modules="\
	glib-2.0 >= $GLIB_VERSION,      \
	gmodule-2.0 >= $GLIB_VERSION,   \
	gio-2.0 >= $GLIB_VERSION,       \
	gtk+-2.0 >= $GTK_VERSION,       \
	gstreamer-0.10 >= $GSTREAMER_VERSION \
	"
PKG_CHECK_MODULES(PACKAGE, $pkg_modules)
PACKAGE_CFLAGS="\
	$PACKAGE_CFLAGS \
	$TOPAZ_CFLAGS \
	-DGEL_DISABLE_DEPRECATED -DGEL_IO_DISABLE_DEPRECATED -DGEL_UI_DISABLE_DEPRECATED \
	-Wall -Werror -std=c99 $BUILD_MODE_CFLAGS"

AC_PATH_PROG(CURL_CONFIG, curl-config)
if test x$CURL_CONFIG = 'x'; then
	echo "curl-config not found"
	exit 1
fi

CURL_CFLAGS="`$CURL_CONFIG --cflags`"
CURL_LIBS="`$CURL_CONFIG --libs`"

PACKAGE_CFLAGS="$PACKAGE_CFLAGS $CURL_CFLAGS"
PACKAGE_LIBS="$PACKAGE_LIBS $CURL_LIBS"
# PACKAGE_LIBS="--as-needed $PACKAGE_LIBS"

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
AC_SUBST(GTK_VERSION)
AC_SUBST(GSTREAMER_VERSION)

dnl
dnl Optional deps: clutter, notify, ige-mac-integration, dbus-glib, sqlite3, unique, uuid
dnl

dnl
dnl Clutter (core, fieshta)
dnl
CLUTTER_VERSION="99.10.0"
clutter_modules="clutter-gtk-0.10 >= $CLUTTER_VERSION"
PKG_CHECK_MODULES(CLUTTER, $clutter_modules,
[
	AC_SUBST(CLUTTER_CFLAGS)
	AC_SUBST(CLUTTER_LIBS)
	HAVE_CLUTTER=yes
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS $CLUTTER_CFLAGS -DHAVE_CLUTTER=1"
	PACKAGE_LIBS="$PACKAGE_LIBS $CLUTTER_LIBS"
],
[
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_CLUTTER=0"
	HAVE_CLUTTER=no
])
AM_CONDITIONAL(USE_CLUTTER, test x"$HAVE_CLUTTER" = "xyes")

if test x$enable_clutty = "xyes" -a x"$HAVE_CLUTTER" = "xyes"; then
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_CLUTTY=1"
else
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_CLUTTY=0"
fi

dnl
dnl notify (ntfy)
dnl
NOTIFY_VERSION="0.4.0"
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
	]
	)

dnl
dnl ige-mac-integracion (ige)
dnl
IGE_VERSION="0.8.0"
ige_modules="ige-mac-integration >= $IGE_VERSION"
if test `pkg-config --variable target gtk+-2.0` = "quartz"; then
	PKG_CHECK_MODULES(IGE, $ige_modules,
		[
			HAVE_IGE=yes
			PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_IGE=1"
			AC_SUBST(IGE_CFLAGS)
			AC_SUBST(IGE_LIBS)
		],
		[
			PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_IGE=0"
			HAVE_IGE=no
		]
		)
else
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_IGE=0"
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
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_DBUS_GLIB=1"
	],
	[
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_DBUS_GLIB=0"
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
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_SQLITE3=1"
	AC_SUBST(SQLITE3_CFLAGS)
	AC_SUBST(SQLITE3_LIBS)
else
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_SQLITE3=0"
fi

dnl
dnl unique (core)
dnl
UNIQUE_VERSION="1.0"
unique_modules="unique-1.0 >= $UNIQUE_VERSION"
PKG_CHECK_MODULES(UNIQUE, $unique_modules,
	[
		AC_SUBST(UNIQUE_CFLAGS)
		AC_SUBST(UNIQUE_LIBS)
		HAVE_UNIQUE=yes
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS $UNIQUE_CFLAGS -DHAVE_UNIQUE=1"
		PACKAGE_LIBS="$PACKAGE_LIBS $UNIQUE_LIBS"
	],
	[
		PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_UNIQUE=0"
		HAVE_UNIQUE=no
	]
	)

dnl
dnl uuid (!core, callhome)
dnl
UUID_VERSION="2.16.0"
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
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_UUID=1"
	AC_SUBST(UUID_CFLAGS)
	AC_SUBST(UUID_LIBS)
else
	PACKAGE_CFLAGS="$PACKAGE_CFLAGS -DHAVE_UUID=0"
fi

dnl
dnl Conditionals
dnl
AM_CONDITIONAL(BUILD_ADB,      test x"$HAVE_SQLITE3"   = "xyes")
AM_CONDITIONAL(BUILD_CALLHOME, test x"$HAVE_UUID"      = "xyes")
AM_CONDITIONAL(USE_CLUTTY,     test x"$enable_clutty"  = "xyes" -a x"$HAVE_CLUTTER" = "xyes")
AM_CONDITIONAL(BUILD_DBUS,     test x"$HAVE_DBUS_GLIB" = "xyes")
AM_CONDITIONAL(BUILD_IGE,      test x"$HAVE_IGE"       = "xyes")
AM_CONDITIONAL(BUILD_NTFY,     test x"$HAVE_NOTIFY"    = "xyes")
AM_CONDITIONAL(BUILD_UNIQUE,   test x"$HAVE_UNIQUE"    = "xyes")

dnl Keep in alphabetical order
AC_CONFIG_FILES([
bundle/build
bundle/Info.plist
data/eina.desktop
data/eina-0.9.pc
Makefile
docs/Makefile
icons/Makefile
icons/hicolor/Makefile
po/Makefile.in
tools/Makefile
])
AC_OUTPUT

