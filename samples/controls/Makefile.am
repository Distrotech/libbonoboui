bin_PROGRAMS = bonobo-sample-controls

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(BONOBOUI_CFLAGS)

bonobo_sample_controls_SOURCES =			\
	bonobo-sample-controls.c

bonobo_sample_controls_LDADD =				\
	$(top_builddir)/bonobo/libbonoboui-2.la		\
	$(BONOBOUI_LIBS)				\
	$(INTLLIBS)

serverdir = $(libdir)/bonobo/servers
server_in_files = Bonobo_Sample_Controls.server.in
server_DATA = $(server_in_files:.server.in=.server)

@XML_I18N_MERGE_SERVER_RULE@

EXTRA_DIST = $(server_in_files) $(server_DATA)
