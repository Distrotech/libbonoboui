/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME control frame object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-control.h>
#include <bonobo/gnome-control-frame.h>
#include <gdk/gdkprivate.h>

enum {
	REQUEST_RESIZE,
	ACTIVATE_URI,
	LAST_SIGNAL
};

static guint control_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_control_frame_parent_class;

/* The entry point vectors for the server we provide */
POA_GNOME_ControlFrame__epv  gnome_control_frame_epv;
POA_GNOME_ControlFrame__vepv gnome_control_frame_vepv;

struct _GnomeControlFramePrivate {
	GNOME_Control	 control;
};

static void
impl_GNOME_ControlFrame_request_resize (PortableServer_Servant servant,
					const CORBA_short new_width,
					const CORBA_short new_height,
					CORBA_Environment *ev)
{
	GnomeControlFrame *control_frame = GNOME_CONTROL_FRAME (gnome_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [REQUEST_RESIZE],
			 (gint) new_width,
			 (gint) new_height);
}

static void
impl_GNOME_ControlFrame_activate_uri (PortableServer_Servant servant,
				      const CORBA_char *uri,
				      CORBA_boolean relative,
				      CORBA_Environment *ev)
{
	GnomeControlFrame *control_frame = GNOME_CONTROL_FRAME (gnome_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control_frame),
			 control_frame_signals [ACTIVATE_URI],
			 (const char *) uri, (gboolean) relative);
}

static CORBA_Object
create_gnome_control_frame (GnomeObject *object)
{
	POA_GNOME_ControlFrame *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_ControlFrame *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_control_frame_vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_ControlFrame__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return gnome_object_activate_servant (object, servant);
}

/**
 * gnome_control_frame_construct:
 * @control_frame: The GnomeControlFrame object to be initialized.
 * @corba_control_frame: A CORBA object for the GNOME_ControlFrame interface.
 *
 * Initializes @control_frame with the parameters.
 *
 * Returns: the initialized GnomeControlFrame object @control_frame that implements the
 * GNOME::ControlFrame CORBA service.
 */
GnomeControlFrame *
gnome_control_frame_construct (GnomeControlFrame *control_frame,
			       GNOME_ControlFrame corba_control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTROL_FRAME (control_frame), NULL);

	gnome_object_construct (GNOME_OBJECT (control_frame), corba_control_frame);
	
	return control_frame;
}

/**
 * gnome_control_frame_new:
 *
 * Returns: GnomeControlFrame object that implements the
 * GNOME::ControlFrame CORBA service.
 */
GnomeControlFrame *
gnome_control_frame_new (void)
{
	GNOME_ControlFrame corba_control_frame;
	GnomeControlFrame *control_frame;
	
	control_frame = gtk_type_new (GNOME_CONTROL_FRAME_TYPE);

	corba_control_frame = create_gnome_control_frame (GNOME_OBJECT (control_frame));
	if (corba_control_frame == CORBA_OBJECT_NIL) {
		gtk_object_destroy (GTK_OBJECT (control_frame));
		return NULL;
	}
	
	return gnome_control_frame_construct (control_frame, corba_control_frame);
}

static void
gnome_control_frame_destroy (GtkObject *object)
{
	GnomeControlFrame *control_frame = GNOME_CONTROL_FRAME (object);

	if (control_frame->priv->control != CORBA_OBJECT_NIL){
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		GNOME_Control_unref (control_frame->priv->control, &ev);
		CORBA_Object_release (control_frame->priv->control, &ev);
		CORBA_exception_free (&ev);
	}
	
	g_free (control_frame->priv);
	
	GTK_OBJECT_CLASS (gnome_control_frame_parent_class)->destroy (object);
}

static void
init_control_frame_corba_class (void)
{
	/* The entry point vectors for this GNOME::Control class */
	gnome_control_frame_epv.request_resize = &impl_GNOME_ControlFrame_request_resize;
	gnome_control_frame_epv.activate_uri = &impl_GNOME_ControlFrame_activate_uri;
	
	/* Setup the vector of epvs */
	gnome_control_frame_vepv.GNOME_Unknown_epv = &gnome_object_epv;
	gnome_control_frame_vepv.GNOME_ControlFrame_epv = &gnome_control_frame_epv;
}

typedef void (*GnomeSignal_NONE__STRING_BOOL) (GtkObject *, const char *, gboolean, gpointer);

static void
gnome_marshal_NONE__STRING_BOOL (GtkObject     *object,
				 GtkSignalFunc  func,
				 gpointer       func_data,
				 GtkArg        *args)
{
	GnomeSignal_NONE__STRING_BOOL rfunc;

	rfunc = (GnomeSignal_NONE__STRING_BOOL) func;
	(*rfunc)(object, GTK_VALUE_STRING (args [0]), GTK_VALUE_BOOL (args [1]), func_data);
}

static void
gnome_control_frame_class_init (GnomeControlFrameClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_control_frame_parent_class = gtk_type_class (GNOME_OBJECT_TYPE);

	control_frame_signals [REQUEST_RESIZE] =
		gtk_signal_new ("request_resize",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeControlFrameClass, request_resize),
				gtk_marshal_NONE__INT_INT,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_INT, GTK_TYPE_INT);

	control_frame_signals [ACTIVATE_URI] =
		gtk_signal_new ("activate_uri",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeControlFrameClass, activate_uri),
				gnome_marshal_NONE__STRING_BOOL,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_STRING, GTK_TYPE_BOOL);
	
	gtk_object_class_add_signals (
		object_class,
		control_frame_signals,
		LAST_SIGNAL);

	object_class->destroy = gnome_control_frame_destroy;

	init_control_frame_corba_class ();
}

static void
gnome_control_frame_init (GnomeObject *object)
{
	GnomeControlFrame *control_frame = GNOME_CONTROL_FRAME (object);

	control_frame->priv = g_new0 (GnomeControlFramePrivate, 1);
}

/**
 * gnome_control_frame_get_type:
 *
 * Returns: The GtkType for the GnomeControlFrame class.
 */
GtkType
gnome_control_frame_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"GnomeControlFrame",
			sizeof (GnomeControlFrame),
			sizeof (GnomeControlFrameClass),
			(GtkClassInitFunc) gnome_control_frame_class_init,
			(GtkObjectInitFunc) gnome_control_frame_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_control_frame_bind_to_control:
 * @control_frame: A GnomeControlFrame object.
 * @control: The CORBA object for the GnomeControl embedded
 * in this ViewFrame.
 *
 * Associates @control with this @control_frame.
 */
void
gnome_control_frame_bind_to_control (GnomeControlFrame *control_frame, GNOME_Control control)
{
	CORBA_Environment ev;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (GNOME_IS_CONTROL_FRAME (control_frame));

	CORBA_exception_init (&ev);
	GNOME_Control_ref (control, &ev);
	control_frame->priv->control = CORBA_Object_duplicate (control, &ev);
	CORBA_exception_free (&ev);
}

