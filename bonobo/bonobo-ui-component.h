/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_UI_COMPONENT_H_
#define _BONOBO_UI_COMPONENT_H_

BEGIN_GNOME_DECLS

#include <bonobo/bonobo-object.h>

#define BONOBO_UI_COMPONENT_TYPE        (bonobo_ui_component_get_type ())
#define BONOBO_UI_COMPONENT(o)          (GTK_CHECK_CAST ((o), BONOBO_UI_COMPONENT_TYPE, BonoboUIComponent))
#define BONOBO_UI_COMPONENT_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_UI_COMPONENT_TYPE, BonoboUIComponentClass))
#define BONOBO_IS_UI_COMPONENT(o)       (GTK_CHECK_TYPE ((o), BONOBO_UI_COMPONENT_TYPE))
#define BONOBO_IS_UI_COMPONENT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_UI_COMPONENT_TYPE))

typedef struct _BonoboUIComponent BonoboUIComponent;
typedef struct _BonoboUIComponentPrivate BonoboUIComponentPrivate;

typedef void (*BonoboUIListenerFn) (BonoboUIComponent           *component,
				    const char                  *path,
				    Bonobo_UIComponent_EventType type,
				    const char                  *state,
				    gpointer                     user_data);

typedef void (*BonoboUIVerbFn)    (BonoboUIComponent           *component,
				   gpointer                     user_data,
				   const char                  *cname);

struct _BonoboUIComponent {
	BonoboObject              object;
	BonoboUIComponentPrivate *priv;
};

typedef struct {
	BonoboObjectClass         parent_class;

	void                    (*exec_verb) (BonoboUIComponent           *comp,
					      const char                  *cname);

	void                    (*ui_event)  (BonoboUIComponent           *comp,
					      const char                  *path,
					      Bonobo_UIComponent_EventType type,
					      const char                  *state);

} BonoboUIComponentClass;

GtkType            bonobo_ui_component_get_type     (void);

BonoboUIComponent *bonobo_ui_component_construct    (BonoboUIComponent  *component,
						     Bonobo_UIComponent  corba_ui,
						     const char         *name);

BonoboUIComponent *bonobo_ui_component_new          (const char         *name);

void               bonobo_ui_component_add_verb     (BonoboUIComponent  *component,
						     const char         *cname,
						     BonoboUIVerbFn      fn,
						     gpointer            user_data);

void               bonobo_ui_component_add_listener (BonoboUIComponent  *component,
						     const char         *id,
						     BonoboUIListenerFn  fn,
						     gpointer            user_data);

void               bonobo_ui_component_set          (BonoboUIComponent  *component,
						     Bonobo_UIContainer  container,
						     const char         *path,
						     const char         *xml,
						     CORBA_Environment  *ev);

void               bonobo_ui_component_set_tree     (BonoboUIComponent  *component,
						     Bonobo_UIContainer  container,
						     const char         *path,
						     xmlNode            *node,
						     CORBA_Environment  *ev);

void               bonobo_ui_component_rm           (BonoboUIComponent  *component,
						     Bonobo_UIContainer  container,
						     const char         *path,
						     CORBA_Environment  *ev);

char              *bonobo_ui_container_get          (Bonobo_UIContainer  container,
						     const char         *path,
						     gboolean            recurse,
						     CORBA_Environment  *ev);

xmlNode           *bonobo_ui_container_get_tree     (Bonobo_UIContainer  container,
						     const char         *path,
						     gboolean            recurse,
						     CORBA_Environment  *ev);

void               bonobo_ui_container_object_set   (Bonobo_UIContainer  container,
						     const char         *path,
						     Bonobo_Unknown      control,
						     CORBA_Environment  *ev);

Bonobo_Unknown     bonobo_ui_container_object_get   (Bonobo_UIContainer  container,
						     const char         *path,
						     CORBA_Environment  *opt_ev);

void               bonobo_ui_container_freeze       (Bonobo_UIContainer  container,
						     CORBA_Environment  *opt_ev);

void               bonobo_ui_container_thaw         (Bonobo_UIContainer  container,
						     CORBA_Environment  *opt_ev);

void               bonobo_ui_component_set_prop     (BonoboUIComponent  *componetn,
						     Bonobo_UIContainer  container,
						     const char         *path,
						     const char         *prop,
						     const char         *value,
						     CORBA_Environment  *opt_ev);

gchar             *bonobo_ui_component_get_prop     (Bonobo_UIContainer  container,
						     const char         *path,
						     const char         *prop,
						     CORBA_Environment  *opt_ev);


POA_Bonobo_UIComponent__epv *bonobo_ui_component_get_epv (void);
Bonobo_UIComponent bonobo_ui_component_corba_object_create (BonoboObject *object);

typedef struct {
	char          *cname;
	BonoboUIVerbFn cb;
	gpointer       user_data;
} BonoboUIVerb;

#define BONOBO_UI_VERB(name,cb)           { (name), ((BonoboUIVerbFn)(cb)), NULL   }
#define BONOBO_UI_VERB_DATA(name,cb,data) { (name), ((BonoboUIVerbFn)(cb)), (data) }
#define BONOBO_UI_VERB_END                { NULL, NULL, NULL }

void    bonobo_ui_component_add_verb_list           (BonoboUIComponent  *component,
						     BonoboUIVerb       *list);
void    bonobo_ui_component_add_verb_list_with_data (BonoboUIComponent  *component,
						     BonoboUIVerb       *list,
						     gpointer            user_data);

END_GNOME_DECLS

#endif /* _BONOBO_UI_COMPONENT_H_ */
