/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_CONTROL_H_
#define _GNOME_CONTROL_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-control-frame.h>

BEGIN_GNOME_DECLS
 
#define GNOME_CONTROL_TYPE        (gnome_control_get_type ())
#define GNOME_CONTROL(o)          (GTK_CHECK_CAST ((o), GNOME_CONTROL_TYPE, GnomeControl))
#define GNOME_CONTROL_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_CONTROL_TYPE, GnomeControlClass))
#define GNOME_IS_CONTROL(o)       (GTK_CHECK_TYPE ((o), GNOME_CONTROL_TYPE))
#define GNOME_IS_CONTROL_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_CONTROL_TYPE))

typedef struct _GnomeControl GnomeControl;
typedef struct _GnomeControlPrivate GnomeControlPrivate;
typedef struct _GnomeControlClass GnomeControlClass;

struct _GnomeControl {
	GnomeObject base;

	GNOME_ControlFrame control_frame;
	GnomeControlPrivate *priv;
};

struct _GnomeControlClass {
	GnomeObjectClass parent_class;
	/*
	 * Signals
	 */
	void (*size_query)               (GnomeControl *control, int *desired_width, int *desired_height);
};

GtkType		   gnome_control_get_type            (void);
GnomeControl	  *gnome_control_construct	     (GnomeControl *control,
						      GNOME_Control corba_control,
						      GtkWidget *widget);

GNOME_Control      gnome_control_corba_object_create (GnomeObject *object);

GnomeControl      *gnome_control_new                 (GtkWidget *widget);

void		   gnome_control_set_control_frame   (GnomeControl *control,
						      GNOME_ControlFrame control_frame);
GNOME_ControlFrame gnome_control_get_control_frame   (GnomeControl *control);

void               gnome_control_request_resize      (GnomeControl *control,
						      int width, int height);

/* CORBA default vector methods we provide */
extern POA_GNOME_Control__epv gnome_control_epv;
extern POA_GNOME_Control__vepv gnome_control_vepv;

END_GNOME_DECLS

#endif /* _GNOME_CONTROL_H_ */
