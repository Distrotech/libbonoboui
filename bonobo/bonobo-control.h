/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_CONTROL_H_
#define _BONOBO_CONTROL_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-control-frame.h>
#include <bonobo/bonobo-property-bag.h>
#include <bonobo/bonobo-property-bag-client.h>

BEGIN_GNOME_DECLS
 
#define BONOBO_CONTROL_TYPE        (bonobo_control_get_type ())
#define BONOBO_CONTROL(o)          (GTK_CHECK_CAST ((o), BONOBO_CONTROL_TYPE, BonoboControl))
#define BONOBO_CONTROL_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_CONTROL_TYPE, BonoboControlClass))
#define BONOBO_IS_CONTROL(o)       (GTK_CHECK_TYPE ((o), BONOBO_CONTROL_TYPE))
#define BONOBO_IS_CONTROL_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_CONTROL_TYPE))

typedef struct _BonoboControl BonoboControl;
typedef struct _BonoboControlPrivate BonoboControlPrivate;
typedef struct _BonoboControlClass BonoboControlClass;

struct _BonoboControl {
	BonoboObject base;

	BonoboControlPrivate *priv;
};

struct _BonoboControlClass {
	BonoboObjectClass parent_class;

	/*
	 * Signals.
	 */
	void (*set_frame)           (BonoboControl *control);
	void (*activate)            (BonoboControl *control, gboolean state);
	void (*undo_last_operation) (BonoboControl *control);
};

/* The main API */
BonoboControl              *bonobo_control_new                     (GtkWidget     *widget);
GtkWidget                  *bonobo_control_get_widget              (BonoboControl *control);
void                        bonobo_control_set_automerge           (BonoboControl *control,
								    gboolean       automerge);
gboolean                    bonobo_control_get_automerge           (BonoboControl *control);
void                        bonobo_control_set_menus               (BonoboControl *control,
								    GnomeUIInfo   *menus);
void                        bonobo_control_set_menus_with_data     (BonoboControl *control,
								    GnomeUIInfo   *menus,
								    gpointer       closure);
BonoboUIHandlerMenuItem    *bonobo_control_get_menus               (BonoboControl *control);
void                        bonobo_control_set_toolbars            (BonoboControl *control,
								    GnomeUIInfo   *toolbars);
void                        bonobo_control_set_toolbars_with_data  (BonoboControl *control,
								    GnomeUIInfo   *toolbars,
								    gpointer       closure);
BonoboUIHandlerToolbarItem *bonobo_control_get_toolbars            (BonoboControl *control);


void                        bonobo_control_set_property            (BonoboControl       *control,
								    const char          *first_prop,
								    ...);
void                        bonobo_control_get_property            (BonoboControl       *control,
								    const char          *first_prop,
								    ...);

/* "Internal" stuff */
GtkType                     bonobo_control_get_type                (void);
BonoboControl              *bonobo_control_construct               (BonoboControl       *control,
								    Bonobo_Control       corba_control,
								    GtkWidget           *widget);
Bonobo_Control              bonobo_control_corba_object_create     (BonoboObject        *object);
void                        bonobo_control_set_control_frame       (BonoboControl       *control,
								    Bonobo_ControlFrame  control_frame);
Bonobo_ControlFrame         bonobo_control_get_control_frame       (BonoboControl       *control);
BonoboUIHandler            *bonobo_control_get_ui_handler          (BonoboControl       *control);
void                        bonobo_control_set_property_bag        (BonoboControl       *control,
								    BonoboPropertyBag   *pb);
BonoboPropertyBag          *bonobo_control_get_property_bag        (BonoboControl       *control);
Bonobo_UIHandler            bonobo_control_get_remote_ui_handler   (BonoboControl       *control);
Bonobo_PropertyBag          bonobo_control_get_ambient_properties  (BonoboControl       *control,
								    CORBA_Environment   *ev);
void                        bonobo_control_activate_notify         (BonoboControl       *control,
								    gboolean             activated);
Bonobo_Control_windowid     bonobo_control_windowid_from_x11       (guint32              x11_id);
POA_Bonobo_Control__epv    *bonobo_control_get_epv                 (void);

END_GNOME_DECLS

#endif /* _BONOBO_CONTROL_H_ */
