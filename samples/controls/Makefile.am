bin_PROGRAMS = bonobo-sample-controls

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(BONOBOUI_CFLAGS)

bonobo_sample_controls_SOURCES =			\
	bonobo-sample-controls.c

bonobo_sample_controls_LDADD =				\
	$(top_builddir)/bonobo/libbonoboui-2.la		\
	$(BONOBOUI_LIBS)				\
	$(INTLLIBS)

oafdir = $(datadir)/oaf
oaf_in_files = Bonobo_Sample_Controls.oaf.in
oaf_DATA = $(oaf_in_files:.oaf.in=.oaf)

@XML_I18N_MERGE_OAF_RULE@

EXTRA_DIST = $(oaf_in_files) $(oaf_DATA)
