/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_CONTROL_FRAME_H_
#define _BONOBO_CONTROL_FRAME_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-wrapper.h>
#include <bonobo/bonobo-property-bag-client.h>
#include <bonobo/bonobo-ui-handler.h>

BEGIN_GNOME_DECLS
 
#define BONOBO_CONTROL_FRAME_TYPE        (bonobo_control_frame_get_type ())
#define BONOBO_CONTROL_FRAME(o)          (GTK_CHECK_CAST ((o), BONOBO_CONTROL_FRAME_TYPE, BonoboControlFrame))
#define BONOBO_CONTROL_FRAME_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_CONTROL_FRAME_TYPE, BonoboControlFrameClass))
#define BONOBO_IS_CONTROL_FRAME(o)       (GTK_CHECK_TYPE ((o), BONOBO_CONTROL_FRAME_TYPE))
#define BONOBO_IS_CONTROL_FRAME_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_CONTROL_FRAME_TYPE))

typedef struct _BonoboControlFramePrivate BonoboControlFramePrivate;
typedef struct _BonoboControlFrame BonoboControlFrame;

struct _BonoboControlFrame {
	BonoboObject base;
	BonoboControlFramePrivate *priv;
};

typedef struct {
	BonoboObjectClass parent_class;

	/*
	 * Signals.
	 */
	void (*activated)           (BonoboControlFrame *control_frame, gboolean state);
	void (*activate_uri)        (BonoboControlFrame *control_frame, const char *uri, gboolean relative);
	void (*undo_last_operation) (BonoboControlFrame *view_frame);

} BonoboControlFrameClass;


GtkType                      bonobo_control_frame_get_type                  (void);
BonoboControlFrame           *bonobo_control_frame_construct                 (BonoboControlFrame  *control_frame,
									    Bonobo_ControlFrame  corba_control_frame);
BonoboControlFrame           *bonobo_control_frame_new                       (void);
void                         bonobo_control_frame_bind_to_control           (BonoboControlFrame  *control_frame,
									    Bonobo_Control       control);
Bonobo_Control                bonobo_control_frame_get_control               (BonoboControlFrame  *control_frame);
void			     bonobo_control_frame_set_propbag		   (BonoboControlFrame  *control_frame,
									    BonoboPropertyBag   *propbag);
BonoboPropertyBag	    *bonobo_control_frame_get_propbag		   (BonoboControlFrame  *control_frame);
GtkWidget                   *bonobo_control_frame_get_widget                (BonoboControlFrame  *frame);
void                         bonobo_control_frame_control_activate          (BonoboControlFrame *control_frame);
void                         bonobo_control_frame_control_deactivate        (BonoboControlFrame *control_frame);
void                         bonobo_control_frame_set_ui_handler            (BonoboControlFrame     *view_frame,
									    BonoboUIHandler        *uih);
BonoboUIHandler              *bonobo_control_frame_get_ui_handler            (BonoboControlFrame  *view_frame);
BonoboPropertyBagClient      *bonobo_control_frame_get_control_property_bag  (BonoboControlFrame  *control_frame);
POA_Bonobo_ControlFrame__epv *bonobo_control_frame_get_epv                   (void);

/*
 * A BonoboControlFrame acts as a proxy for the remote BonoboControl object to
 * which it is bound.  These functions act as wrappers which a
 * container can use to communicate with the BonoboControl associated with
 * a given BonoboControlFrame.
 */
void  bonobo_control_frame_size_request (BonoboControlFrame *control_frame,
					int *desired_width,
					int *desired_height);
    
/* The entry point vectors for the server we provide */
extern POA_Bonobo_ControlFrame__epv  bonobo_control_frame_epv;
extern POA_Bonobo_ControlFrame__vepv bonobo_control_frame_vepv;

END_GNOME_DECLS

#endif /* _BONOBO_CONTROL_FRAME_H_ */
