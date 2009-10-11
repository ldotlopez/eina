fieshtadir = $(libdir)/$(PACKAGE)/fieshta

fieshta_LTLIBRARIES = libfieshta.la

AM_CFLAGS = @PACKAGE_CFLAGS@ 
libfieshta_la_LDFLAGS = @PACKAGE_LIBS@ -module -avoid-version
libfieshta_la_SOURCES = fieshta.c fieshta.h

if USE_CLUTTER
libfieshta_la_SOURCES += fieshta-stage.c fieshta-stage.h fieshta-stream.c fieshta-stream.h
AM_CFLAGS += @CLUTTER_CFLAGS@
libfieshta_la_LDFLAGS += @CLUTTER_LIBS@
endif
