## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=bonobo

# The top-level SGML file.
DOC_MAIN_SGML_FILE=bonobo-api.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(top_srcdir)/bonobo

CFLAGS="-I../.. `gnome-config --cflags bonobo`"
LDFLAGS=`gnome-config --libs bonobo`

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

tmpl_sources =

bonobo_docdir = $(HTML_DIR)
bonobo_doc_DATA = \
	bonobo-api.sgml		\
	bonobo-api.html		\
	bonobo.hierarchy	\
	bonobo.signals		\
	bonobo.types		\
	bonobo-decl.txt		\
	bonobo-sections.txt

EXTRA_DIST = $(bonobo_doc_DATA)

if ENABLE_GTK_DOC
bonobo-api.html: html/book1.html
	-cp $(srcdir)/html/book1.html $(srcdir)/bonobo-api.html
else
bonobo-api.html:
endif

html/book1.html: sgml/bonobo-api-doc.bottom
	$(MAKE) html

sgml/bonobo-api-doc.bottom: $(tmpl_sources)
	$(MAKE) sgml

scan:
	-(cd $(srcdir) \
	&& env CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) \
		 gtkdoc-scanobj --module=$(DOC_MODULE) \
	&& gtkdoc-scan --module=$(DOC_MODULE) \
		../../bonobo/bonobo-object.h ../../bonobo/bonobo-moniker.h \
		../../bonobo/bonobo-exception.h \
		../../bonobo/bonobo-item-container.h ../../bonobo/bonobo-object-client.h \
		../../bonobo/bonobo-moniker-util.h ../../bonobo/bonobo-property-listener.h \
		../../bonobo/bonobo-property-bag-client.h ../../bonobo/bonobo-control.h \
		../../bonobo/bonobo-control-frame.h ../../bonobo/bonobo-view.h ../../bonobo/bonobo-generic-factory.h \
		../../bonobo/bonobo-embeddable.h ../../bonobo/bonobo-embeddable-factory.h \
		../../bonobo/bonobo-sizeable.h ../../bonobo/bonobo-main.h ../../bonobo/bonobo-stream.h \
		../../bonobo/bonobo-stream-client.h ../../bonobo/bonobo-stream-fs.h ../../bonobo/bonobo-persist.h \
		../../bonobo/bonobo-persist-file.h ../../bonobo/bonobo-persist-stream.h ../../bonobo/bonobo-ui-xml.h \
		../../bonobo/bonobo-ui-util.h ../../bonobo/bonobo-ui-component.h ../../bonobo/bonobo-ui-container.h \
		../../bonobo/bonobo-win.h ../../bonobo/bonobo-object-io.h ../../bonobo/bonobo-progressive.h \
		../../bonobo/bonobo-storage.h ../../bonobo/bonobo-selector.h ../../bonobo/bonobo-widget.h \
		../../bonobo/bonobo-print.h ../../bonobo/bonobo-print-client.h )


templates: scan
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	cd $(srcdir) \
	&& gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=..

html:
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	-cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

clean-local:
	rm -f *~ *.bak *-unused.txt

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	  echo '-- Fixing Crossreferences' ; \
	  gtkdoc-fixxref --module=$(DOC_MODULE) --html-dir=$(HTML_DIR)|| true; \
	fi)

dist-hook:
	mkdir $(distdir)/html
	mkdir $(distdir)/sgml
	mkdir $(distdir)/tmpl
	-cp $(srcdir)/html/*.html $(srcdir)/html/*.css $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/sgml/*.bottom $(srcdir)/sgml/*.top $(distdir)/sgml

.PHONY : html sgml templates scan

