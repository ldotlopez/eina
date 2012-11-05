#
# Init package
#
AC_INIT([Eina],[0.15.0],[https://bugs.launchpad.net/eina],[eina],[http://eina.sourceforge.net/])
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
GTK_DOC_CHECK(1.9)
AM_PATH_PYTHON

#
# Package information
#
GETTEXT_PACKAGE="eina"
EINA_API_VERSION="0.16"
EINA_VERSION="0.15.0"
EINA_CODENAME="Dead bodies everywhere"    # git codename
EINA_CODENAME_0_14_0="Too drunk to fuck"  # Nouveau vague
EINA_CODENAME_0_12_0="Rise"               # Primal scream
EINA_CODENAME_0_10_0="La primavera"       # Delafe y las flores azules
EINA_CODENAME_0_9_4="Trained"             # Najwa
EINA_CODENAME_0_9_3="White rabbit"        # Jefferson airplane
EINA_CODENAME_0_9_2="Pagan poetry"        # Björk
EINA_CODENAME_0_9_1="In muine we believe" #
EINA_CODENAME_0_9_0="They don't believe"  # Russian Red

EINA_URL="http://eina.sourceforge.net/"
EINA_URL_HELP="http://eina.sourceforge.net/"
EINA_URL_BUGS="https://bugs.launchpad.net/eina"
EINA_ONELINE_COPYRIGHT="Copyright 2004-2012 Eina authors"
EINA_AUTHORS="Eina authors"
AC_SUBST(EINA_URL)
AC_SUBST(EINA_URL_HELP)
AC_SUBST(EINA_URL_BUGS)
AC_SUBST(EINA_ONELINE_COPYRIGHT)
AC_SUBST(EINA_AUTHORS)

GEL_MAJOR=2
GEL_MINOR=0
GEL_MICRO=0
GEL_API_VERSION="$GEL_MAJOR.$GEL_MINOR"
GEL_VERSION="$GEL_API_VERSION.$GEL_MICRO"
AC_SUBST(GEL_API_VERSION)
AC_SUBST(GEL_VERSION)
AC_SUBST(GEL_MAJOR)
AC_SUBST(GEL_MINOR)
AC_SUBST(GEL_MICRO)
LOMO_MAJOR=2
LOMO_MINOR=0
LOMO_MICRO=0
LOMO_API_VERSION="$LOMO_MAJOR.$LOMO_MINOR"
LOMO_VERSION="$LOMO_API_VERSION.$LOMO_MICRO"
AC_SUBST(LOMO_API_VERSION)
AC_SUBST(LOMO_VERSION)
AC_SUBST(LOMO_MAJOR)
AC_SUBST(LOMO_MINOR)
AC_SUBST(LOMO_MICRO)

AC_DEFINE_UNQUOTED(EINA_CODENAME,"$EINA_CODENAME", [Eina version codename])
AC_DEFINE_UNQUOTED(EINA_URL, "$EINA_URL", [Eina URL])
AC_DEFINE_UNQUOTED(EINA_URL_HELP, "$EINA_URL_HELP", [Eina help URL])
AC_DEFINE_UNQUOTED(EINA_URL_BUGS, "$EINA_URL_BUGS", [Eina bugs URL])
AC_DEFINE_UNQUOTED(GEL_API_VERSION,  "$GEL_API_VERSION",  [Gel API version] )
AC_DEFINE_UNQUOTED(LOMO_API_VERSION, "$LOMO_API_VERSION", [Lomo API version] )
AC_DEFINE_UNQUOTED(EINA_API_VERSION,"$EINA_API_VERSION",  [Eina API version])

AC_SUBST(EINA_VERSION)
AC_SUBST(EINA_CODENAME)
AC_SUBST(EINA_API_VERSION)

PACKAGE_RELEASE_DATE="`date '+%a, %d %b %Y %H:%m:%S %z'`"
AC_SUBST(PACKAGE_RELEASE_DATE)

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

EINA_APP_DOMAIN="net.sourceforge.eina"
EINA_APP_PATH_DOMAIN="/net/sourceforge/eina"
AC_SUBST(EINA_APP_DOMAIN)
AC_SUBST(EINA_APP_PATH_DOMAIN)
AC_DEFINE_UNQUOTED(EINA_APP_DOMAIN,"$EINA_APP_DOMAIN", [Eina application identifier])
AC_DEFINE_UNQUOTED(EINA_APP_PATH_DOMAIN,"$EINA_APP_PATH_DOMAIN", [Eina application path])

# for po/
DATADIRNAME="share"
AC_SUBST(DATADIRNAME)

dnl aclocal flags
AC_CONFIG_MACRO_DIR([m4])
# ACLOCAL_AMFLAGS="-I build/m4 ${ACLOCAL_FLAGS}"
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl pkgconfig
PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$top_builddir/data"

# Introspection
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
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
#AC_PATH_PROG(VALA_GEN_INTROSPECT, vala-gen-introspect)
#AC_PATH_PROG(VAPIGEN, vapigen)
#AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`

AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(GLIB_COMPILE_SCHEMAS)
AC_SUBST(GDK_PIXBUF_CSOURCE)
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
AC_PROG_CC_C99
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
	-DGTK_MULTIHEAD_SAFE -DGSEAL_ENABLE"

AC_ARG_ENABLE(debug,        [  --enable-debug              Build with enabled debug (+debug,-optimizations)], enable_debug=yes, enable_debug=no)
AC_ARG_ENABLE(experimental, [  --enable-experimental       Compile with experimental features], enable_experimental=yes, enable_experimental=no)

if test x$enable_debug = 'xyes'; then
	DEBUG_CFLAGS="-O0 -g"
else
	DEBUG_CFLAGS="-O2"
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
PEAS_API_VERSION="1.0"

GLIB_VERSION="2.28.0"
GTK_VERSION="3.0.0"
GST_VERSION="$GST_API_VERSION.0"
PEAS_VERSION="0.7.3"
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
peas_modules="\
	libpeas-$PEAS_API_VERSION >= $PEAS_VERSION \
	libpeas-gtk-$PEAS_API_VERSION >= $PEAS_VERSION \
	"
PKG_CHECK_MODULES(GLIB, $glib_modules)
PKG_CHECK_MODULES(GTK,  $gtk_modules)
PKG_CHECK_MODULES(GST,  $gst_modules)
PKG_CHECK_MODULES(PEAS, $peas_modules)

GDK_QUARTZ_BACKEND="`pkg-config --variable=target gdk-3.0`"
AC_DEFINE([GDK_QUARTZ_BACKEND], [1], [Define if GDK is using the quartz backend])

AC_SUBST(GLIB_API_VERSION)
AC_SUBST(GTK_API_VERSION)
AC_SUBST(GST_API_VERSION)

EINA_CFLAGS="\
	$GLIB_CFLAGS         \
	$GTK_CFLAGS          \
	$GST_CFLAGS          \
	$PEAS_CFLAGS         \
	$COMMON_CFLAGS       \
	$EXPERIMENTAL_CFLAGS \
	"
EINA_LIBS="\
	$GLIB_LIBS         \
	$GTK_LIBS          \
	$GST_LIBS          \
	$PEAS_LIBS         \
	$COMMON_LIBS       \
	$EXPERIMENTAL_LIBS \
	-L\$(top_builddir)/gel/.libs -lgel-$GEL_API_VERSION -lgel-ui-$GEL_API_VERSION \
	-L\$(top_builddir)/lomo/.libs -llomo-$LOMO_API_VERSION \
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

HAVE_OSX_OS="no"
case "$host_os" in
*darwin* | *rhapsody* | *macosx*)
	AC_DEFINE_UNQUOTED(OS_OSX, 1, [This is an OS X system])
	HAVE_OSX_OS="yes"
	;;
linux*)
	AC_DEFINE_UNQUOTED(OS_LINUX, 1, [This is a Linux system])
	;;
*)
	AC_DEFINE_UNQUOTED(OS_UNKNOW, 1, [This is an unknow system])
	;;
esac

dnl
dnl Clutter (core, fieshta)
dnl
AC_ARG_WITH([clutter-gtk],
    [AS_HELP_STRING([--with-clutter-gtk],
        [disable support for clutter-gtk])],
    [],
    [with_clutter_gtk=yes])
AS_IF([test "x$with_clutter_gtk" != xno],
[CLUTTER_GTK_VERSION="1.0.0"
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
])])
AM_CONDITIONAL(USE_CLUTTER_GTK, test x"$HAVE_CLUTTER_GTK" = "xyes")

dnl
dnl clastfm
dnl
AC_ARG_WITH([clastfm],
    [AS_HELP_STRING([--with-clastfm],
        [disable support for clastfm])],
    [],
    [with_clastfm=yes])
AS_IF([test "x$with_clastfm" != xno],
[CLASTFM_VERSION="0.4"
clastfm_modules="libclastfm >= $CLASTFM_VERSION"
PKG_CHECK_MODULES(CLASTFM, $clastfm_modules,
[
	AC_SUBST(CLASTFM_CFLAGS)
	AC_SUBST(CLASTFM_LIBS)
	HAVE_CLASTFM=yes
	EINA_CFLAGS="$EINA_CFLAGS $CLASTFM_CFLAGS -DHAVE_CLASTFM=1"
	EINA_LIBS="$EINA_LIBS $CLASTFM_LIBS"
],
[
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_CLASTFM=0"
	HAVE_CLASTFM=no
])])
AM_CONDITIONAL(USE_CLASTFM, test x"$HAVE_CLASTFM" = "xyes")

dnl
dnl notify (ntfy)
dnl
AC_ARG_WITH([libnotify],
    [AS_HELP_STRING([--with-libnotify],
        [disable support for libnotify])],
    [],
    [with_libnotify=yes])
AS_IF([test "x$with_libnotify" != xno],
[notify_modules="libnotify >= 0.7.0"
PKG_CHECK_MODULES(NOTIFY, $notify_modules,
[
	AC_SUBST(NOTIFY_CFLAGS)
	AC_SUBST(NOTIFY_LIBS)
	HAVE_NOTIFY=yes
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_NOTIFY=1 -DNOTIFY_VERSION_0_7"
],
[
	notify_modules="libnotify >= 0.5.0"
	PKG_CHECK_MODULES(NOTIFY, $notify_modules,
	[
		AC_SUBST(NOTIFY_CFLAGS)
		AC_SUBST(NOTIFY_LIBS)
		HAVE_NOTIFY=yes
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_NOTIFY=1 -DNOTIFY_VERSION_0_5"
	],
	[
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_NOTIFY=0"
		HAVE_NOTIFY=no
	])
])])

dnl
dnl indicate (!mpris)
dnl
AC_ARG_WITH([indicate],
    [AS_HELP_STRING([--with-indicate],
        [disable support for indicate])],
    [],
    [with_indicate=yes])
AS_IF([test "x$with_indicate" != xno],
[
INDICATE_VERSION="0.5.0"
indicate_modules="indicate-0.5 >= $INDICATE_VERSION"
PKG_CHECK_MODULES(INDICATE, $indicate_modules,
	[
		AC_SUBST(INDICATE_CFLAGS)
		AC_SUBST(INDICTE_LIBS)
		EINA_CFLAGS="$EINA_CFLAGS -DHAVE_INDICATE=1"
		HAVE_INDICATE=yes
	],
	[
		INDICATE_VERSION="0.4.0"
		indicate_modules="indicate >= $INDICATE_VERSION"
		PKG_CHECK_MODULES(INDICATE, $indicate_modules,
		[
			AC_SUBST(INDICATE_CFLAGS)
			AC_SUBST(INDICTE_LIBS)
			EINA_CFLAGS="$EINA_CFLAGS -DHAVE_INDICATE=1"
			HAVE_INDICATE=yes
		],
		[
			HAVE_INDICATE=no
		])
	])])

dnl
dnl sqlite3 (!core, adb)
dnl
AC_ARG_WITH([sqlite3],
    [AS_HELP_STRING([--with-sqlite3],
        [disable support for sqlite3])],
    [],
    [with_sqlite3=yes])
AS_IF([test "x$with_sqlite3" != xno],
[
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
	)])
if test x"$HAVE_SQLITE3" = "xyes"; then
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_SQLITE3=1"
	AC_SUBST(SQLITE3_CFLAGS)
	AC_SUBST(SQLITE3_LIBS)
else
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_SQLITE3=0"
fi

dnl
dnl gtk-mac-integration
dnl
GTKMAC_VERSION="1.0.0"
gtkmac_modules="gtk-mac-integration >= $GTKMAC_VERSION"
PKG_CHECK_MODULES(GTKMAC, $gtkmac_modules, [ HAVE_GTKMAC=yes ], [ HAVE_GTKMAC=no ])
if test x"$HAVE_GTKMAC" = "xyes"; then
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_GTKMAC=1"
	AC_SUBST(GTKMAC_CFLAGS)
	AC_SUBST(GTKMAC_LIBS)
else
	EINA_CFLAGS="$EINA_CFLAGS -DHAVE_GTKMAC=0"
fi

dnl
dnl Conditionals
dnl
AM_CONDITIONAL(BUILD_ADB,      test x"$HAVE_SQLITE3"     = "xyes")
AM_CONDITIONAL(BUILD_CALLHOME, test x"$HAVE_UUID"        = "xyes")
AM_CONDITIONAL(BUILD_HIPSTER,  test x"$HAVE_CLASTFM"     = "xyes")
AM_CONDITIONAL(BUILD_CLUTTY,   test x"$HAVE_CLUTTER_GTK" = "xyes")
AM_CONDITIONAL(BUILD_INDICATE, test x"$HAVE_INDICATE"    = "xyes")
AM_CONDITIONAL(BUILD_NTFY,     test x"$HAVE_NOTIFY"      = "xyes")
AM_CONDITIONAL(BUILD_GTKMAC,   test x"$HAVE_GTKMAC"      = "xyes")

dnl Keep in alphabetical order
AC_CONFIG_FILES([
build/Makefile
bundle/build
bundle/Info.plist
data/Makefile
data/eina.desktop
data/gel-${GEL_API_VERSION}.pc
data/gel-ui-${GEL_API_VERSION}.pc
data/lomo-${LOMO_API_VERSION}.pc
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
eina/resources.xml
eina/core/Makefile
eina/data/Makefile
eina/adb/adb.plugin
eina/adb/Makefile
eina/clutty/clutty.plugin
eina/clutty/Makefile
eina/dbus/dbus.plugin
eina/dbus/Makefile
eina/dock/dock.plugin
eina/dock/Makefile
eina/fieshta/fieshta.plugin
eina/fieshta/Makefile
eina/hipster/hipster.plugin
eina/hipster/Makefile
eina/importer/importer.plugin
eina/importer/Makefile
eina/lastfm/lastfm.plugin
eina/lastfm/resources.xml
eina/lastfm/Makefile
eina/lomo/lomo.plugin
eina/lomo/Makefile
eina/lyrics/lyrics.plugin
eina/lyrics/Makefile
eina/mpris/mpris.plugin
eina/mpris/Makefile
eina/muine/muine.plugin
eina/muine/resources.xml
eina/muine/Makefile
eina/ntfy/ntfy.plugin
eina/ntfy/Makefile
eina/osx/osx.plugin
eina/osx/Makefile
eina/player/player.plugin
eina/player/resources.xml
eina/player/Makefile
eina/playlist/playlist.plugin
eina/playlist/resources.xml
eina/playlist/Makefile
eina/preferences/preferences.plugin
eina/preferences/Makefile
eina/pythonconsole/pythonconsole.plugin
eina/pythonconsole/Makefile
eina/pythonhello/pythonhello.plugin
eina/pythonhello/Makefile
eina/status-icon/status-icon.plugin
eina/status-icon/Makefile
eina/stars/stars.plugin
eina/stars/Makefile
Makefile
lomo/Makefile
po/Makefile.in
osx/Makefile
osx/Info.plist
])
AC_OUTPUT

