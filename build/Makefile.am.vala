vala_stamps = $(foreach vala_file,$(vala_files),$(vala_file).stamp)
vala_h_files = $(vala_files:%.vala=%.h)
vala_c_files = $(vala_files:%.vala=%.c)

BUILT_SOURCES  += $(vala_stamps)
CLEANFILES     += $(vala_stamps) $(vala_c_files) $(vala_h_files)
DISTCLEANFILES += $(vala_stamps) $(vala_c_files) $(vala_h_files)
EXTRA_DIST += $(vala_files) $(vala_c_files) $(vala_h_files)

%.vala.stamp: %.vala
	$(AM_V_GEN)$(VALAC) --header $(patsubst %.vala,%.h,$<) --use-header --ccode $(AM_VALAFLAGS) $(VALAFLAGS) $< \
		&& touch $@

