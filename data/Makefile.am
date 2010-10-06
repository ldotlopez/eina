include $(top_srcdir)/build/Makefile.am.common

# pkgconfig files
data_pkgconfigdir = $(libdir)/pkgconfig
data_pkgconfig_DATA = eina.pc

# desktop files
data_applicationsdir = $(datadir)/applications
data_applications_DATA = eina.desktop

@INTLTOOL_DESKTOP_RULE@

install-am:
	test -z "$(DESTDIR)/$(prefix)/share/applications" || /bin/mkdir -p "$(DESTDIR)/$(prefix)/share/applications"
	install -c -m 644 eina.desktop "$(DESTDIR)/$(prefix)/share/applications/$(PACKAGE).desktop"
	
	test -z "$(DESTDIR)/$(prefix)/lib/pkgconfig" || /bin/mkdir -p "$(DESTDIR)/$(prefix)/lib/pkgconfig"
	install -c -m 644 eina.pc "$(DESTDIR)/$(prefix)/lib/pkgconfig/$(PACKAGE)-$(EINA_API_VERSION).pc"
