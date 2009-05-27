## Process this file with automake to produce Makefile.in

# AUTOMAKE_OPTIONS = 1.8

SUBDIRS =                 \
	misc icons pixmaps ui       \
	gel lomo eina plugins \
	po

#DIST_SUBDIRS = \
#	$(SUBDIRS)

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	COPYRIGHT README AUTHORS INSTALL

#intltool_extra = intltool-extract.in intltool-merge.in intltool-update.in
#
#EXTRA_DIST = $(intltool_extra)  MAINTAINERS
#DISTCLEANFILES =        \
#	intltool-extract    \
#	intltool-merge      \
#	intltool-update

#appsdir = $(datadir)/applications
#apps_DATA = eina.desktop
#
#schemasdir = $(sysconfdir)/gconf/schemas
#schemas_DATA = eina.schemas

#EXTRA_DIST = \
#	intltool-update.in  \
#	intltool-extract.in \
#	intltool-merge.in

#install-data-local:
#	@$(NORMAL_INSTALL)
#	$(mkinstalldirs) $(DESTDIR)$(datadir)/applications
#	$(INSTALL_DATA) $(srcdir)/eina.desktop $(DESTDIR)$(datadir)/applications/eina.desktop
#	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)
#	$(mkinstalldirs) $(DESTDIR)$(datadir)/application-registry
#	$(mkinstalldirs) $(DESTDIR)$(datadir)/mime-info
#	$(INSTALL_DATA) $(srcdir)/player.glade $(DESTDIR)$(datadir)/$(PACKAGE)/player.glade;
#	$(INSTALL_DATA) $(srcdir)/eina.applications $(DESTDIR)$(datadir)/application-registry/eina.applications;
#	$(INSTALL_DATA) $(srcdir)/eina.keys $(DESTDIR)$(datadir)/mime-info/eina.keys;
#	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schemas_DATA)

