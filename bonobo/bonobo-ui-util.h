#ifndef _BONOBO_UI_XML_UTIL_H_
#define _BONOBO_UI_XML_UTIL_H_

#include <gnome-xml/tree.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <bonobo/bonobo-ui-component.h>

void       bonobo_ui_util_build_skeleton       (BonoboUIXml  *xml);
char      *bonobo_ui_util_pixbuf_to_xml        (GdkPixbuf    *pixbuf);

GdkPixbuf *bonobo_ui_util_xml_to_pixbuf  (const char *xml);

GdkPixbuf *bonobo_ui_util_xml_get_icon_pixbuf         (BonoboUINode *node);
GtkWidget *bonobo_ui_util_xml_get_icon_pixmap_widget  (BonoboUINode *node);

void  bonobo_ui_util_xml_set_pixbuf     (BonoboUINode  *node,
					 GdkPixbuf     *pixbuf);
void  bonobo_ui_util_xml_set_pix_xpm    (BonoboUINode  *node,
					 const char   **xpm);
void  bonobo_ui_util_xml_set_pix_stock  (BonoboUINode  *node,
					 const char    *name);
void  bonobo_ui_util_xml_set_pix_fname  (BonoboUINode  *node,
					 const char    *name);

void       bonobo_ui_util_build_help_menu   (BonoboUIComponent *listener,
					     const char        *app_name,
					     BonoboUINode           *parent);

BonoboUINode   *bonobo_ui_util_build_accel       (guint              accelerator_key,
					     GdkModifierType    accelerator_mods,
					     const char        *verb);

BonoboUINode   *bonobo_ui_util_new_menu          (gboolean           submenu,
					     const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *verb);

BonoboUINode   *bonobo_ui_util_new_placeholder   (const char        *name,
					     gboolean           top,
					     gboolean           bottom);

void       bonobo_ui_util_set_radiogroup    (BonoboUINode           *node,
					     const char        *group_name);

void       bonobo_ui_util_set_toggle        (BonoboUINode           *node,
					     const char        *id,
					     const char        *init_state);

BonoboUINode   *bonobo_ui_util_new_std_toolbar   (const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *verb);
					     
BonoboUINode   *bonobo_ui_util_new_toggle_toolbar(const char        *name,
					     const char        *label,
					     const char        *descr,
					     const char        *id);

char      *bonobo_ui_util_get_ui_fname      (const char        *component_prefix,
					     const char        *file_name);

void       bonobo_ui_util_translate_ui      (BonoboUINode      *node);

void       bonobo_ui_util_fixup_help        (BonoboUIComponent *component,
					     BonoboUINode           *node,
					     const char        *app_name);

/*
 * Does all the translation & other grunt.
 */
BonoboUINode   *bonobo_ui_util_new_ui       (BonoboUIComponent *component,
					     const char        *fname,
					     const char        *app_name);

void            bonobo_ui_util_set_ui       (BonoboUIComponent *component,
					     Bonobo_UIContainer container,
					     const char        *component_prefix,
					     const char        *file_name,
					     const char        *app_name);

void            bonobo_ui_util_set_pixbuf   (Bonobo_UIContainer container,
					     const char        *path,
					     GdkPixbuf         *pixbuf);

gchar     *bonobo_ui_util_accel_name        (guint              accelerator_key,
					     GdkModifierType    accelerator_mods);

void       bonobo_ui_util_accel_parse       (char              *name,
					     guint             *accelerator_key,
					     GdkModifierType   *accelerator_mods);

#endif /* _BONOBO_UI_XML_UTIL_H_ */
