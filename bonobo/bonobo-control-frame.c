/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME control frame object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-control.h>
#include <bonobo/gnome-control-frame.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdktypes.h>
#include <gtk/gtksocket.h>

enum {
	ACTIVATE_URI,
	LAST_SIGNAL
};

static guint control_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_control_frame_parent_class;

/* The entry point vectors for the server we provide */
POA_GNOME_ControlFrame__vepv gnome_control_frame_vepv;

struct _GnomeControlFramePrivate {
	GNOME_Control	 control;
	GtkWidget	*socket;
};

static void
impl_GNOME_ControlFrame_queue_resize (PortableServer_Servant servant,
				      CORBA_Environment *ev)
{
	/*
	 * Nothing.
	 *
	 * In the Gnome implementation of Bonobo, all size negotiation
	 * is handled by GtkPlug/GtkSocket for us.
	 */
}

static void
impl_GNOME_ControlFrame_activate_uri (PortableServer_Servant servant,
				      CORBA_char *uri,
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

static void
gnome_control_frame_set_remote_window (GtkWidget *socket, GnomeControlFrame *control_frame)
{
	GNOME_Control control = gnome_control_frame_get_control (control_frame);
	CORBA_Environment ev;
	GNOME_Control_windowid id;

	/*
	 * If we are not yet bound to a remote control, don't do
	 * anything.
	 */
	if (control == CORBA_OBJECT_NIL)
		return;

	/*
	 * Otherwise, pass the window ID of our GtkSocket to the
	 * remote Control.
	 */
	CORBA_exception_init (&ev);
	id = gnome_control_windowid_from_x11 (GDK_WINDOW_XWINDOW (socket->window));
	GNOME_Control_set_window (control, id, &ev);
	g_free (id);
	if (ev._major != CORBA_NO_EXCEPTION)
		gnome_object_check_env (GNOME_OBJECT (control_frame), control, &ev);
	CORBA_exception_free (&ev);
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

	/*
	 * Now create the GtkSocket which will be used to embed
	 * the Control.
	 */
	control_frame->priv->socket = gtk_socket_new ();
	gtk_widget_show (control_frame->priv->socket);

	/*
	 * When the socket is realized, we pass its Window ID to our
	 * Control.
	 */
	gtk_signal_connect (GTK_OBJECT (control_frame->priv->socket),
			    "realize",
			    GTK_SIGNAL_FUNC (gnome_control_frame_set_remote_window),
			    control_frame);

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

/**
 * gnome_control_frame_get_epv:
 */
POA_GNOME_ControlFrame__epv *
gnome_control_frame_get_epv (gboolean duplicate)
{
	static POA_GNOME_ControlFrame__epv cf_epv = {
		NULL,
		&impl_GNOME_ControlFrame_queue_resize,
		&impl_GNOME_ControlFrame_activate_uri
	};
	POA_GNOME_ControlFrame__epv *epv;

	if(duplicate) {
		epv = g_new0 (POA_GNOME_ControlFrame__epv, 1);
		memcpy(epv, &cf_epv, sizeof(cf_epv));
	} else
		epv = &cf_epv;

	return epv;
}

static void
init_control_frame_corba_class (void)
{
	/* Setup the vector of epvs */
	gnome_control_frame_vepv.GNOME_Unknown_epv = gnome_object_get_epv (FALSE);
	gnome_control_frame_vepv.GNOME_ControlFrame_epv = gnome_control_frame_get_epv (FALSE);
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
 * Returns: The GtkType for the GnomeControlFrame class.  */
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
 * in this GnomeControlFrame.
 *
 * Associates @control with this @control_frame.
 */
void
gnome_control_frame_bind_to_control (GnomeControlFrame *control_frame, GNOME_Control control)
{
	CORBA_Environment ev;

	g_return_if_fail (control_frame != NULL);
	g_return_if_fail (GNOME_IS_CONTROL_FRAME (control_frame));

	/*
	 * Keep a local handle to the Control.
	 */
	CORBA_exception_init (&ev);
	GNOME_Control_ref (control, &ev);
	control_frame->priv->control = CORBA_Object_duplicate (control, &ev);
	CORBA_exception_free (&ev);

	/*
	 * Introduce ourselves to the Control.
	 */
	CORBA_exception_init (&ev);
	GNOME_Control_set_frame (control,
				 gnome_object_corba_objref (GNOME_OBJECT (control_frame)),
				 &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		gnome_object_check_env (GNOME_OBJECT (control_frame), control, &ev);
	CORBA_exception_free (&ev);

	/*
	 * If the socket is realized, then we transfer the
	 * window ID to the remote control.
	 */
	if (GTK_WIDGET_REALIZED (control_frame->priv->socket)) {
		gnome_control_frame_set_remote_window (control_frame->priv->socket,
						       control_frame);
	}
}

/**
 * gnome_control_frame_get_control:
 * @control_frame: A GnomeControlFrame which is bound to a remote
 * GnomeControl.
 *
 * Returns: The GNOME_Control CORBA interface for the remote Control
 * which is bound to @frame.  See also
 * gnome_control_frame_bind_to_control().
 */
GNOME_Control
gnome_control_frame_get_control (GnomeControlFrame *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_CONTROL_FRAME (control_frame), CORBA_OBJECT_NIL);

	return control_frame->priv->control;
}

/**
 * gnome_control_frame_get_widget:
 * @frame: The GnomeControlFrame whose widget is being requested.a
 *
 * Use this function when you want to embed a GnomeControl into your
 * container's widget hierarchy.  Once you have bound the
 * GnomeControlFrame to a remote GnomeControl, place the widget
 * returned by gnome_control_frame_get_widget() into your widget
 * hierarchy and the control will appear in your application.
 *
 * Returns: A GtkWidget which has the remote GnomeControl physically
 * inside it.
 */
GtkWidget *
gnome_control_frame_get_widget (GnomeControlFrame *control_frame)
{
	g_return_val_if_fail (control_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTROL_FRAME (control_frame), NULL);

	return control_frame->priv->socket;
}


/**
 * gnome_control_frame_get_control_property_bag:
 */
GnomePropertyBagClient *
gnome_control_frame_get_control_property_bag (GnomeControlFrame *control_frame)
{
	GNOME_PropertyBag pbag;
	CORBA_Environment ev;
	GNOME_Control control;

	control = control_frame->priv->control;

	CORBA_exception_init (&ev);

	pbag = GNOME_Control_get_property_bag (control, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (GNOME_OBJECT (control_frame), control, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);

	return gnome_property_bag_client_new (pbag);
}
