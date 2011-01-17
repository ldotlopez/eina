include $(top_srcdir)/build/Makefile.am.common

# pkgconfig files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	gel-$(GEL_API_VERSION).pc    \
	gel-ui-$(GEL_API_VERSION).pc \
	lomo-$(LOMO_API_VERSION).pc  \
	eina-$(EINA_API_VERSION).pc

# desktop files
applicationsdir = $(datadir)/applications
applications_DATA = eina.desktop
@INTLTOOL_DESKTOP_RULE@

# schemas
gsettings_SCHEMAS = net.sourceforge.eina.gschema.xml
gsettingsdir = $(datadir)/glib-2.0/schemas
gsettings_DATA = $(gsettings_SCHEMAS)
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
	test -z "$(DESTDIR)/$(applicationsdir)" || /bin/mkdir -p "$(DESTDIR)/$(applicationsdir)"
	install -c -m 644 eina.desktop "$(DESTDIR)/$(applicationsdir)/$(PACKAGE).desktop"
	
	test -z "$(DESTDIR)/$(pkgconfigdir)" || /bin/mkdir -p "$(DESTDIR)/$(pkgconfigdir)"
	install -c -m 644 gel-$(GEL_API_VERSION).pc    "$(DESTDIR)/$(pkgconfigdir)/gel-$(GEL_API_VERSION).pc"
	install -c -m 644 gel-ui-$(GEL_API_VERSION).pc "$(DESTDIR)/$(pkgconfigdir)/gel-ui-$(GEL_API_VERSION).pc"
	install -c -m 644 lomo-$(LOMO_API_VERSION).pc  "$(DESTDIR)/$(pkgconfigdir)/lomo-$(LOMO_API_VERSION).pc"
	install -c -m 644 eina-$(EINA_API_VERSION).pc  "$(DESTDIR)/$(pkgconfigdir)/$(PACKAGE)-$(EINA_API_VERSION).pc"

