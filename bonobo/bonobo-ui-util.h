#ifndef _BONOBO_UI_XML_UTIL_H_
#define _BONOBO_UI_XML_UTIL_H_

#include <gnome-xml/tree.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <bonobo/bonobo-ui-component.h>

void       bonobo_ui_util_build_skeleton    (BonoboUIXml *xml);

char      *bonobo_ui_util_pixbuf_to_xml     (GdkPixbuf   *pixbuf);
GdkPixbuf *bonobo_ui_util_xml_to_pixbuf     (const char  *xml);

GtkWidget *bonobo_ui_util_xml_get_pixmap    (GtkWidget   *window,
					     xmlNode     *node);

void       bonobo_ui_util_xml_set_pixbuf    (xmlNode     *node,
					     GdkPixbuf   *pixbuf);

void       bonobo_ui_util_xml_set_pix_xpm   (xmlNode     *node,
					     const char **xpm);
				     
void       bonobo_ui_util_xml_set_pix_stock (xmlNode     *node,
					     const char  *name);

void       bonobo_ui_util_xml_set_pix_fname (xmlNode     *node,
					     const char  *name);

void       bonobo_ui_util_build_help_menu   (BonoboUIComponent *listener,
					     const char        *app_name,
					     xmlNode           *parent);

xmlNode   *bonobo_ui_util_build_accel       (guint              accelerator_key,
					     GdkModifierType    accelerator_mods,
					     const char        *verb);

xmlNode   *bonobo_ui_util_new_menu          (gboolean           submenu,
					     const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *verb);

xmlNode   *bonobo_ui_util_new_placeholder   (const char        *name,
					     gboolean           top,
					     gboolean           bottom);

void       bonobo_ui_util_set_radiogroup    (xmlNode           *node,
					     const char        *group_name);

void       bonobo_ui_util_set_toggle        (xmlNode           *node,
					     const char        *id,
					     const char        *init_state);

xmlNode   *bonobo_ui_util_new_std_toolbar   (const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *verb);
					     
xmlNode   *bonobo_ui_util_new_toggle_toolbar(const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *id);

char      *bonobo_ui_util_get_ui_fname      (const char        *component_name);

void       bonobo_ui_util_translate_ui      (xmlNode           *node);

void       bonobo_ui_util_fixup_help        (BonoboUIComponent *component,
					     xmlNode           *node,
					     const char        *app_name);

/*
 * Does all the translation & other grunt.
 */
xmlNode   *bonobo_ui_util_new_ui            (BonoboUIComponent *component,
					     const char        *fname,
					     const char        *app_name);

gchar     *bonobo_ui_util_accel_name        (guint              accelerator_key,
					     GdkModifierType    accelerator_mods);

void       bonobo_ui_util_accel_parse       (char              *name,
					     guint             *accelerator_key,
					     GdkModifierType   *accelerator_mods);

#endif /* _BONOBO_UI_XML_UTIL_H_ */
