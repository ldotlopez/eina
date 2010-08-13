include $(top_srcdir)/Makefile.am.common

# pkgconfig files
data_pkgconfigdir = $(libdir)/pkgconfig
data_pkgconfig_DATA = eina-@EINA_API_VERSION@.pc

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

