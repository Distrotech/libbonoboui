#
# Makefile for the sample Echo server/client Bonobo components.
#

bin_PROGRAMS     = bonobo-echo echo-client
noinst_LIBRARIES = libEcho.a

if OAF
OBJECT_DIRECTORY_LIBS=$(OAF_LIBS) $(GNOMEUI_LIBS)
else
OBJECT_DIRECTORY_LIBS=$(GNOMEGNORBA_LIBS)
endif

bonobo_echo_SOURCES =		\
	main.c			\
	echo.c			\
	echo.h

bonobo_echo_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(GTK_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(OBJECT_DIRECTORY_LIBS)                \
	libEcho.a				\
	$(INTLLIBS)

echo_client_LDADD =				\
	$(top_builddir)/bonobo/libbonobo.la	\
	$(GTK_LIBS)				\
	$(GNOME_LIBDIR)				\
	$(OBJECT_DIRECTORY_LIBS)                \
	libEcho.a				\
	$(INTLLIBS)

ECHO_CORBA_GENERATED =		\
	Echo-common.c		\
	Echo-skels.c		\
	Echo-stubs.c		\
	Echo.h

libEcho_a_SOURCES = $(ECHO_CORBA_GENERATED)

$(ECHO_CORBA_GENERATED): Echo.idl 
	orbit-idl -I`$(GNOME_CONFIG) --datadir`/idl -I$(top_srcdir)/idl $(srcdir)/Echo.idl
	touch my_echo_idl


goaddir = $(sysconfdir)/CORBA/servers
oafdir = $(datadir)/oaf

GOAD_FILES = echo.gnorba
OAF_FILES = echo.oafinfo

#if OAF
goad_DATA = $(GOAD_FILES)
#else
oaf_DATA = $(OAF_FILES)
#endif


idldir    = $(datadir)/idl
idl_DATA  = Echo.idl

EXTRA_DIST = $(GOAD_FILES) $(OAF_FILES) $(idl_DATA)

INCLUDES = 						\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DG_LOG_DOMAIN=\"Echo\"				\
	-I. -I$(top_srcdir)/bonobo			\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
        -I$(includedir)                                 \
        $(GNOME_INCLUDEDIR)

MYHACKAROUNDAUTOMAKE=$(shell echo $(ECHO_CORBA_GENERATED))



