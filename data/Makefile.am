include $(top_srcdir)/build/Makefile.am.common

# pkgconfig files
data_pkgconfigdir = $(libdir)/pkgconfig
data_pkgconfig_DATA = eina.pc

# desktop files
data_applicationsdir = $(datadir)/applications
data_applications_DATA = eina.desktop
@INTLTOOL_DESKTOP_RULE@

# schemas
gsettings_SCHEMAS = net.sourceforge.eina.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@
DISTCLEANFILES += \
	$(gsettings_SCHEMAS)
CLEANFILES += \
	$(gsettings_SCHEMAS)
EXTRA_DIST += \
	$(gsettings_SCHEMAS)
MAINTAINERCLEANFILES += $(gsettings_SCHEMAS:.xml=.valid)

install-am:
	test -z "$(DESTDIR)/$(prefix)/share/applications" || /bin/mkdir -p "$(DESTDIR)/$(prefix)/share/applications"
	install -c -m 644 eina.desktop "$(DESTDIR)/$(prefix)/share/applications/$(PACKAGE).desktop"
	
	test -z "$(DESTDIR)/$(prefix)/lib/pkgconfig" || /bin/mkdir -p "$(DESTDIR)/$(prefix)/lib/pkgconfig"
	install -c -m 644 eina.pc "$(DESTDIR)/$(prefix)/lib/pkgconfig/$(PACKAGE)-$(EINA_API_VERSION).pc"
	
	test -z "$(DESTDIR)/$(prefix)/share/glib-2.0/schemas/" || \
		/bin/mkdir -p "$(DESTDIR)/$(prefix)/share/glib-2.0/schemas/"
	install -c -m 644 net.sourceforge.eina.gschema.xml \
		"$(DESTDIR)/$(prefix)/share/glib-2.0/schemas/net.sourceforge.eina.gschema.xml"

