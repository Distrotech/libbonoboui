/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_CONTROL_FRAME_H_
#define _GNOME_CONTROL_FRAME_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-wrapper.h>
#include <bonobo/gnome-property-bag-client.h>
#include <bonobo/gnome-ui-handler.h>

BEGIN_GNOME_DECLS
 
#define GNOME_CONTROL_FRAME_TYPE        (gnome_control_frame_get_type ())
#define GNOME_CONTROL_FRAME(o)          (GTK_CHECK_CAST ((o), GNOME_CONTROL_FRAME_TYPE, GnomeControlFrame))
#define GNOME_CONTROL_FRAME_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_CONTROL_FRAME_TYPE, GnomeControlFrameClass))
#define GNOME_IS_CONTROL_FRAME(o)       (GTK_CHECK_TYPE ((o), GNOME_CONTROL_FRAME_TYPE))
#define GNOME_IS_CONTROL_FRAME_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_CONTROL_FRAME_TYPE))

typedef struct _GnomeControlFramePrivate GnomeControlFramePrivate;
typedef struct _GnomeControlFrame GnomeControlFrame;

struct _GnomeControlFrame {
	GnomeObject base;
	GnomeControlFramePrivate *priv;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Signals.
	 */
	void (*activated)           (GnomeControlFrame *control_frame, gboolean state);
	void (*activate_uri)        (GnomeControlFrame *control_frame, const char *uri, gboolean relative);
	void (*undo_last_operation) (GnomeControlFrame *view_frame);

} GnomeControlFrameClass;


GtkType                      gnome_control_frame_get_type                  (void);
GnomeControlFrame           *gnome_control_frame_construct                 (GnomeControlFrame  *control_frame,
									    GNOME_ControlFrame  corba_control_frame);
GnomeControlFrame           *gnome_control_frame_new                       (void);
void                         gnome_control_frame_bind_to_control           (GnomeControlFrame  *control_frame,
									    GNOME_Control       control);
GNOME_Control                gnome_control_frame_get_control               (GnomeControlFrame  *control_frame);
void			     gnome_control_frame_set_propbag		   (GnomeControlFrame  *control_frame,
									    GnomePropertyBag   *propbag);
GnomePropertyBag	    *gnome_control_frame_get_propbag		   (GnomeControlFrame  *control_frame);
GtkWidget                   *gnome_control_frame_get_widget                (GnomeControlFrame  *frame);
void                         gnome_control_frame_set_ui_handler            (GnomeControlFrame     *view_frame,
									    GnomeUIHandler     *uih);
void                         gnome_control_frame_control_activate          (GnomeControlFrame *control_frame);
void                         gnome_control_frame_control_deactivate        (GnomeControlFrame *control_frame);

GnomeUIHandler              *gnome_control_frame_get_ui_handler            (GnomeControlFrame  *view_frame);
GnomePropertyBagClient      *gnome_control_frame_get_control_property_bag  (GnomeControlFrame  *control_frame);
POA_GNOME_ControlFrame__epv *gnome_control_frame_get_epv                   (void);

/*
 * A GnomeControlFrame acts as a proxy for the remote GnomeControl object to
 * which it is bound.  These functions act as wrappers which a
 * container can use to communicate with the GnomeControl associated with
 * a given GnomeControlFrame.
 */
void  gnome_control_frame_size_request (GnomeControlFrame *control_frame,
					int *desired_width,
					int *desired_height);
    
/* The entry point vectors for the server we provide */
extern POA_GNOME_ControlFrame__epv  gnome_control_frame_epv;
extern POA_GNOME_ControlFrame__vepv gnome_control_frame_vepv;

END_GNOME_DECLS

#endif /* _GNOME_CONTROL_FRAME_H_ */
