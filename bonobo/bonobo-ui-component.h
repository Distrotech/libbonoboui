/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_UI_COMPONENT_H_
#define _BONOBO_UI_COMPONENT_H_

BEGIN_GNOME_DECLS

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
				   const char                  *cname,
				   gpointer                     user_data);

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
						     Bonobo_UIComponent  corba_ui);

BonoboUIComponent *bonobo_ui_component_new          (void);

void               bonobo_ui_component_add_verb     (BonoboUIComponent  *component,
						     const char         *cname,
						     const char         *label,
						     const char         *descr,
						     BonoboUIVerbFn      fn,
						     gpointer            user_data);

void               bonobo_ui_component_add_listener (BonoboUIComponent  *component,
						     const char         *id,
						     BonoboUIListenerFn  fn,
						     gpointer            user_data);

POA_Bonobo_UIComponent__epv *bonobo_ui_component_get_epv (void);
Bonobo_UIComponent bonobo_ui_component_corba_object_create (BonoboObject *object);

END_GNOME_DECLS

#endif /* _BONOBO_UI_COMPONENT_H_ */
