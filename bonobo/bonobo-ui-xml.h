/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_UI_XML_H_
#define _BONOBO_UI_XML_H_

#include <gtk/gtk.h>

#include <gnome-xml/parser.h>
#include <gnome-xml/parserInternals.h>
#include <gnome-xml/xmlmemory.h>

#define BONOBO_UI_XML_TYPE        (bonobo_ui_xml_get_type ())
#define BONOBO_UI_XML(o)          (GTK_CHECK_CAST ((o), BONOBO_UI_XML_TYPE, BonoboUIXml))
#define BONOBO_UI_XML_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_UI_XML_TYPE, BonoboUIXmlClass))
#define BONOBO_IS_UI_XML(o)       (GTK_CHECK_TYPE ((o), BONOBO_UI_XML_TYPE))
#define BONOBO_IS_UI_XML_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_UI_XML_TYPE))

typedef struct _BonoboUIXml BonoboUIXml;

typedef struct {
	gpointer id;
	gboolean dirty;
	GSList  *overridden;
} BonoboUIXmlData;

typedef gboolean         (*BonoboUIXmlCompareFn)   (BonoboUIXmlData *a,
						    BonoboUIXmlData *b);
typedef BonoboUIXmlData *(*BonoboUIXmlDataNewFn)   (void);
typedef void             (*BonoboUIXmlDataFreeFn)  (BonoboUIXmlData *data);
typedef void             (*BonoboUIXmlOverrideFn)  (BonoboUIXmlData *data);
typedef void             (*BonoboUIXmlReinstateFn) (BonoboUIXmlData *data);
typedef void             (*BonoboUIXmlDumpFn)      (BonoboUIXmlData *data);

struct _BonoboUIXml {
	GtkObject              object;

	BonoboUIXmlCompareFn   compare;
	BonoboUIXmlDataNewFn   data_new;
	BonoboUIXmlDataFreeFn  data_free;
	BonoboUIXmlOverrideFn  override;
	BonoboUIXmlReinstateFn reinstate;
	BonoboUIXmlDumpFn      dump;;

	xmlNode               *root;
	
	gpointer               dummy;
};

typedef struct {
	GtkObjectClass         object_klass;

	void                 (*override)  (xmlNode *node);
	void                 (*reinstate) (xmlNode *node);

	gpointer               dummy;
} BonoboUIXmlClass;

GtkType      bonobo_ui_xml_get_type  (void);

BonoboUIXml *bonobo_ui_xml_new       (BonoboUIXmlCompareFn  compare,
				      BonoboUIXmlDataNewFn  data_new,
				      BonoboUIXmlDataFreeFn data_free,
				      BonoboUIXmlOverrideFn override,
				      BonoboUIXmlReinstateFn reinstate,
				      BonoboUIXmlDumpFn      dump);

/* Nominaly BonoboUIXmlData * */
gpointer         bonobo_ui_xml_get_data  (BonoboUIXml *tree,
					  xmlNode     *node);

void             bonobo_ui_xml_set_dirty (BonoboUIXml *tree,
					  xmlNode     *node,
					  gboolean     dirty);

char            *bonobo_ui_xml_get_path  (xmlNode     *node);

void             bonobo_ui_xml_merge     (BonoboUIXml *tree,
					  const char  *path,
					  xmlNode     *nodes,
					  gpointer     id);

void             bonobo_ui_xml_rm        (BonoboUIXml *tree,
					  const char  *path,
					  gpointer     id);

void             bonobo_ui_xml_dump      (BonoboUIXml *tree,
					  xmlNode     *node,
					  const char  *msg);

#endif /* _BONOBO_UI_XML_H_ */
