#ifndef _BONOBO_UI_XML_UTIL_H_
#define _BONOBO_UI_XML_UTIL_H_

#include <gnome-xml/tree.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

char      *bonobo_ui_util_pixbuf_to_xml  (GdkPixbuf   *pixbuf);
GdkPixbuf *bonobo_ui_util_xml_to_pixbuf  (const char  *xml);

GtkWidget *bonobo_ui_util_xml_get_pixmap (GtkWidget   *window,
					  xmlNode     *node);

void       bonobo_ui_util_xml_set_pixbuf (xmlNode     *node,
					   GdkPixbuf   *pixbuf);

void       bonobo_ui_util_xml_set_xpm    (xmlNode     *node,
					   const char **xpm);
				     
void       bonobo_ui_util_xml_set_stock  (xmlNode     *node,
					   const char  *name);

void       bonobo_ui_util_xml_set_fname  (xmlNode     *node,
					   const char  *name);
				     

#endif /* _BONOBO_UI_XML_UTIL_H_ */
