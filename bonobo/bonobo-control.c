/* -*- mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo control object
 *
 * Author:
 *   Nat Friedman (nat@helixcode.com)
 *   Miguel de Icaza (miguel@helixcode.com)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */
#include <config.h>
#include <stdlib.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-plug.h>
#include <bonobo/bonobo-control.h>
#include <gdk/gdkprivate.h>
#include <bonobo/bonobo-ui-compat.h>

enum {
	SET_FRAME,
	ACTIVATE,
	UNDO_LAST_OPERATION,
	LAST_SIGNAL
};

static guint control_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static BonoboObjectClass *bonobo_control_parent_class;

/* The entry point vectors for the server we provide */
POA_Bonobo_Control__vepv bonobo_control_vepv;

struct _BonoboControlPrivate {
	GtkWidget                  *widget;
	Bonobo_ControlFrame         control_frame;
	gboolean                    active;

	GtkWidget                  *plug;
	gboolean                    is_local;
	gboolean                    xid_received;
			
	BonoboUIHandler            *uih;
	gboolean                    automerge;
				   
	BonoboPropertyBag          *propbag;
};

/**
 * window_id_demangle:
 * @id: CORBA_char *
 * 
 * De-mangle a window id string,
 * fields are separated by ':' character,
 * currently only the first field is used.
 * 
 * Return value: the X11 window id.
 **/
inline static guint32
window_id_demangle (Bonobo_Control_windowid id)
{
	guint32 x11_id;
	char **elements;
	
/*	printf ("ID string '%s'\n", id);*/

	elements = g_strsplit (id, ":", -1);
	if (elements && elements [0])
		x11_id = strtol (elements [0], NULL, 10);
	else {
		g_warning ("Serious X id mangling error");
		x11_id = 0;
	}
	g_strfreev (elements);

/*	printf ("x11 : %d\n", x11_id);*/

	return x11_id;
}

/**
 * bonobo_control_windowid_from_x11:
 * @x11_id: the x11 window id.
 * 
 * This mangles the X11 name into the ':' delimited
 * string format "X-id: ..."
 * 
 * Return value: the string; free after use.
 **/
Bonobo_Control_windowid
bonobo_control_windowid_from_x11 (guint32 x11_id)
{
	CORBA_char *str;

	str = g_strdup_printf ("%d", x11_id);

/*	printf ("Mangled %d to '%s'\n", x11_id, str);*/
	return str;
}

/*
 * This callback is invoked when the plug is unexpectedly destroyed by
 * way of its associated X window dying.  This usually indicates that
 * the contaier application has died.  This callback is _not_ invoked
 * if the BonoboControl is destroyed normally, i.e. the user unrefs
 * the BonoboControl away.
 */
static void
bonobo_control_plug_destroy_event_cb (GtkWidget   *plug,
				      GdkEventAny *event,
				      gpointer     closure)
{
	BonoboControl *control = BONOBO_CONTROL (closure);

	if (control->priv->plug == NULL)
		return;

	if (control->priv->plug != plug)
		g_warning ("Destroying incorrect plug!");

	/*
	 * Set the plug to NULL here so that we don't try to
	 * destroy it later.  It will get destroyed on its
	 * own.
	 */
	control->priv->plug            = NULL;

	/*
	 * Destroy this plug's BonoboControl.
	 */
	bonobo_object_unref (BONOBO_OBJECT (control));
}

/*
 * This callback is invoked when the plug is unexpectedly destroyed
 * through normal Gtk channels. FIXME FIXME FIXME 
 *
 */
static void
bonobo_control_plug_destroy_cb (GtkWidget *plug,
				gpointer   closure)
{
	BonoboControl *control = BONOBO_CONTROL (closure);

	if (control->priv->plug == NULL)
		return;

	if (control->priv->plug != plug)
		g_warning ("Destroying incorrect plug!");

	/*
	 * Set the plug to NULL here so that we don't try to
	 * destroy it later.  It will get destroyed on its
	 * own.
	 */
	control->priv->plug = NULL;
}


static void
bonobo_control_auto_merge (BonoboControl *control)
{
	Bonobo_UIContainer remote_uih;

	if (control->priv->uih == NULL)
		return;

	remote_uih = bonobo_control_get_remote_ui_handler (control);
	if (remote_uih == CORBA_OBJECT_NIL)
		return;

	bonobo_ui_handler_set_container (control->priv->uih, remote_uih);

	bonobo_object_release_unref (remote_uih, NULL);

#ifdef STALE_NOT_USED
	if (control->priv->menus != NULL) {
		bonobo_ui_handler_menu_add_list (
			control->priv->uih, "/", control->priv->menus);
	}
#endif
}


static void
bonobo_control_auto_unmerge (BonoboControl *control)
{
	if (control->priv->uih == NULL)
		return;
	
	bonobo_ui_handler_unset_container (control->priv->uih);
}

static void
impl_Bonobo_Control_activate (PortableServer_Servant servant,
			      CORBA_boolean activated,
			      CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	if (control->priv->automerge && control->priv->active != activated) {
		if (activated)
			bonobo_control_auto_merge (control);
		else
			bonobo_control_auto_unmerge (control);
	}

	gtk_signal_emit (GTK_OBJECT (control), control_signals [ACTIVATE], (gboolean) activated);

	control->priv->active = activated;
}


static void
impl_Bonobo_Control_reactivate_and_undo (PortableServer_Servant servant,
					 CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (control), control_signals [ACTIVATE], TRUE);
	gtk_signal_emit (GTK_OBJECT (control), control_signals [UNDO_LAST_OPERATION]);
}
	
static void
impl_Bonobo_Control_set_frame (PortableServer_Servant servant,
			       Bonobo_ControlFrame frame,
			       CORBA_Environment *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	bonobo_control_set_control_frame (control, frame);
}


static GtkWidget *
bonobo_gtk_widget_from_x11_id (guint32 xid)
{
	GdkWindow *window;
	gpointer data;

	window = gdk_window_lookup (xid);
	
	if (! window) {
		return NULL;
	}

	gdk_window_get_user_data(window, &data);

	if (!GTK_IS_WIDGET (data)) {
		return NULL;
	} else {
		return GTK_WIDGET (data);
	}
}

static void
impl_Bonobo_Control_set_window (PortableServer_Servant   servant,
				Bonobo_Control_windowid  id,
				CORBA_Environment       *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));
	GtkWidget     *local_socket;
	guint32        x11_id;

	g_return_if_fail (control->priv->widget != NULL);

	x11_id = window_id_demangle (id);

	/*
	 * Check to see if this XID is local to the application.  In
	 * that case, we bypass the GtkPlug/GtkSocket mechanism and
	 * embed the control directly into the widget hierarchy.  This
	 * avoids a lot of the problems that Plug/Socket give us.
	 */
	local_socket = bonobo_gtk_widget_from_x11_id (x11_id);

	if (! local_socket) {
		GtkWidget *old_plug;


		old_plug            = control->priv->plug;

		/* Create the new plug */
		control->priv->plug = bonobo_plug_new (x11_id);

		gtk_signal_connect (GTK_OBJECT (control->priv->plug), "destroy_event",
				    GTK_SIGNAL_FUNC (bonobo_control_plug_destroy_event_cb), control);
		gtk_signal_connect (GTK_OBJECT (control->priv->plug), "destroy",
				    GTK_SIGNAL_FUNC (bonobo_control_plug_destroy_cb), control);

		/*
		 * Put the control widget inside the plug.  If we
		 * already have a plug, then reparent the control into
		 * the new plug.
		 */
		if (control->priv->xid_received) {

			if (old_plug != NULL) {
				gtk_object_unref (GTK_OBJECT (old_plug));
			}

			gtk_widget_reparent (control->priv->widget, control->priv->plug);
		} else {
 			gtk_container_add (GTK_CONTAINER (control->priv->plug), control->priv->widget);
		}

		gtk_widget_show (control->priv->plug);
		
	} else {
		GtkWidget *socket_parent;

		if (control->priv->xid_received)
			return;

		control->priv->is_local = TRUE;

		socket_parent = local_socket->parent;
		gtk_widget_hide (local_socket);

		gtk_box_pack_end (GTK_BOX (socket_parent),
				  control->priv->widget,
				  TRUE, TRUE, 0);
	}

	control->priv->xid_received = TRUE;
}

static void
impl_Bonobo_Control_size_allocate (PortableServer_Servant  servant,
				   const CORBA_short       width,
				   const CORBA_short       height,
				   CORBA_Environment      *ev)
{
	/*
	 * Nothing.
	 *
	 * In the Gnome implementation of Bonobo, all size negotiation
	 * is handled by GtkPlug/GtkSocket for us, or GtkFrame in the
	 * local case.
	 */
}

static void
impl_Bonobo_Control_size_request (PortableServer_Servant  servant,
				  CORBA_short            *desired_width,
				  CORBA_short            *desired_height,
				  CORBA_Environment      *ev)
{
	/*
	 * Nothing.
	 */
}

static GtkStateType
bonobo_control_gtk_state_from_corba (const Bonobo_Control_State state)
{
	switch (state) {
	case Bonobo_Control_StateNormal:
		return GTK_STATE_NORMAL;

	case Bonobo_Control_StateActive:
		return GTK_STATE_ACTIVE;

	case Bonobo_Control_StatePrelight:
		return GTK_STATE_PRELIGHT;

	case Bonobo_Control_StateSelected:
		return GTK_STATE_SELECTED;

	case Bonobo_Control_StateInsensitive:
		return GTK_STATE_INSENSITIVE;

	default:
		g_warning ("bonobo_control_gtk_state_from_corba: Unknown state: %d", (gint) state);
		return GTK_STATE_NORMAL;
	}
}

static void
impl_Bonobo_Control_set_state (PortableServer_Servant      servant,
			       const Bonobo_Control_State  state,
			       CORBA_Environment          *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));

	g_return_if_fail (control->priv->widget != NULL);

	gtk_widget_set_state (
		control->priv->widget,
		bonobo_control_gtk_state_from_corba (state));
}

static Bonobo_PropertyBag
impl_Bonobo_Control_get_property_bag (PortableServer_Servant  servant,
				      CORBA_Environment      *ev)
{
	BonoboControl *control = BONOBO_CONTROL (bonobo_object_from_servant (servant));
	Bonobo_PropertyBag corba_propbag;

	if (control->priv->propbag == NULL)
		return CORBA_OBJECT_NIL;

	corba_propbag = bonobo_object_corba_objref (
		BONOBO_OBJECT (control->priv->propbag));

	return bonobo_object_dup_ref (corba_propbag, ev);
}

/**
 * bonobo_control_corba_object_create:
 * @object: the GtkObject that will wrap the CORBA object
 *
 * Creates and activates the CORBA object that is wrapped by the
 * @object BonoboObject.
 *
 * Returns: An activated object reference to the created object
 * or %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_Control
bonobo_control_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_Control *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_Control *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_control_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_Control__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (Bonobo_Control) bonobo_object_activate_servant (object, servant);
}

BonoboControl *
bonobo_control_construct (BonoboControl  *control,
			  Bonobo_Control  corba_control,
			  GtkWidget      *widget)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);
	g_return_val_if_fail (corba_control != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	/*
	 * This sets up the X handler for Bonobo objects.  We basically will
	 * ignore X errors if our container dies (because X will kill the
	 * windows of the container and our container without telling us).
	 */
	bonobo_setup_x_error_handler ();

	bonobo_object_construct (BONOBO_OBJECT (control), corba_control);

	control->priv->widget = GTK_WIDGET (widget);
	gtk_object_ref (GTK_OBJECT (widget));
	gtk_object_sink (GTK_OBJECT (widget));

	control->priv->uih = bonobo_ui_handler_new ();
	control->priv->propbag = NULL;

	return control;
}

/**
 * bonobo_control_new:
 * @widget: a GTK widget that contains the control and will be passed to the
 * container process.
 *
 * This function creates a new BonoboControl object for @widget.
 *
 * Returns: a BonoboControl object that implements the Bonobo::Control CORBA
 * service that will transfer the @widget to the container process.
 */
BonoboControl *
bonobo_control_new (GtkWidget *widget)
{
	BonoboControl *control;
	Bonobo_Control corba_control;
	
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	control = gtk_type_new (bonobo_control_get_type ());

	corba_control = bonobo_control_corba_object_create (BONOBO_OBJECT (control));
	if (corba_control == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (control));
		return NULL;
	}
	
	return bonobo_control_construct (control, corba_control, widget);
}

/**
 * bonobo_control_get_widget:
 * @control: a BonoboControl
 *
 * Returns the GtkWidget associated with a BonoboControl.
 *
 * Return value: the BonoboControl's widget
 **/
GtkWidget *
bonobo_control_get_widget (BonoboControl *control)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	return control->priv->widget;
}

/**
 * bonobo_control_set_automerge:
 * @control: A #BonoboControl.
 * @automerge: Whether or not menus and toolbars should be
 * automatically merged when the control is activated.
 *
 * Sets whether or not the control handles menu/toolbar merging
 * automatically.  If automerge is on, the control will automatically
 * create its menus and toolbars when it is activated and destroy them
 * when it is deactivated.  The menus and toolbars which it merges are
 * specified with bonobo_control_set_menus() and
 * bonobo_control_set_toolbars().
 */
void
bonobo_control_set_automerge (BonoboControl *control,
			      gboolean       automerge)
{
	g_return_if_fail (BONOBO_IS_CONTROL (control));

	control->priv->automerge = automerge;
}

/**
 * bonobo_control_get_automerge:
 * @control: A #BonoboControl.
 *
 * Returns: Whether or not the control is set to automerge its
 * menus/toolbars.  See bonobo_control_set_automerge().
 */
gboolean
bonobo_control_get_automerge (BonoboControl *control)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), FALSE);

	return control->priv->automerge;
}

static void
bonobo_control_destroy (GtkObject *object)
{
	BonoboControl *control = BONOBO_CONTROL (object);
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	if (control->priv->active)
		Bonobo_ControlFrame_activated (control->priv->control_frame,
					       FALSE, &ev);

	CORBA_Object_release (control->priv->control_frame, &ev);

	CORBA_exception_free (&ev);

	/*
	 * If we have a UIHandler, destroy it.
	 */
	if (control->priv->uih != NULL) {
		bonobo_ui_handler_unset_container (control->priv->uih);
		bonobo_object_unref (BONOBO_OBJECT (control->priv->uih));
	}
}

static void
bonobo_control_finalize (GtkObject *object)
{
	BonoboControl *control = BONOBO_CONTROL (object);

	/*
	 * Destroy the control's top-level widget.
	 */
	if (control->priv->widget)
		gtk_object_unref (GTK_OBJECT (control->priv->widget));

	/*
	 * If the plug still exists, destroy it.  The plug might not
	 * exist in the case where the container application died,
	 * taking the plug out with it, or the optimized local case
	 * where the plug/socket mechanism was bypassed.  In the
	 * formaer case, plug_destroy_cb() would have been invoked,
	 * and it would have triggered the destruction of the Control,
	 * which is why we're here now. In the latter case, it's not
	 * needed because there is no plug.  
	 */
	if (control->priv->plug) {
		gtk_object_destroy (GTK_OBJECT (control->priv->plug));
		control->priv->plug = NULL;
	}

	g_free (control->priv);

	GTK_OBJECT_CLASS (bonobo_control_parent_class)->finalize (object);
}

/**
 * bonobo_control_get_epv:
 *
 */
POA_Bonobo_Control__epv *
bonobo_control_get_epv (void)
{
	POA_Bonobo_Control__epv *epv;

	epv = g_new0 (POA_Bonobo_Control__epv, 1);

	epv->reactivate_and_undo = impl_Bonobo_Control_reactivate_and_undo;
	epv->activate            = impl_Bonobo_Control_activate;
	epv->size_allocate       = impl_Bonobo_Control_size_allocate;
	epv->set_window          = impl_Bonobo_Control_set_window;
	epv->set_state           = impl_Bonobo_Control_set_state;
	epv->set_frame           = impl_Bonobo_Control_set_frame;
	epv->size_request        = impl_Bonobo_Control_size_request;
	epv->get_property_bag    = impl_Bonobo_Control_get_property_bag;

	return epv;
}

static void
init_control_corba_class (void)
{
	/* Setup the vector of epvs */
	bonobo_control_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_control_vepv.Bonobo_Control_epv = bonobo_control_get_epv ();
}

/**
 * bonobo_control_set_control_frame:
 * @control: A BonoboControl object.
 * @control_frame: A CORBA interface for the ControlFrame which contains this Controo.
 *
 * Sets the ControlFrame for @control to @control_frame.
 */
void
bonobo_control_set_control_frame (BonoboControl *control, Bonobo_ControlFrame control_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (BONOBO_IS_CONTROL (control));

	CORBA_exception_init (&ev);

	if (control->priv->control_frame != CORBA_OBJECT_NIL)
		CORBA_Object_release (control->priv->control_frame, &ev);
	
	control->priv->control_frame = CORBA_Object_duplicate (control_frame, &ev);
	
	CORBA_exception_free (&ev);

	gtk_signal_emit (GTK_OBJECT (control), control_signals [SET_FRAME]);
}

/**
 * bonobo_control_get_control_frame:
 * @control: A BonoboControl object whose Bonobo_ControlFrame CORBA interface is
 * being retrieved.
 *
 * Returns: The Bonobo_ControlFrame CORBA object associated with @control, this is
 * a CORBA_object_duplicated object.  You need to CORBA_free it when you are
 * done with it.
 */
Bonobo_ControlFrame
bonobo_control_get_control_frame (BonoboControl *control)
{
	Bonobo_ControlFrame control_frame;
	CORBA_Environment ev;
	
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	control_frame = CORBA_Object_duplicate (control->priv->control_frame, &ev);
	CORBA_exception_free (&ev);

	return control_frame;
}

/**
 * bonobo_control_get_ui_handler:
 * @control: A BonoboControl object for which a BonoboUIHandler has been
 * created and set.
 *
 * Returns: The #BonoboUIHandler which @control is using.
 */
BonoboUIHandler *
bonobo_control_get_ui_handler (BonoboControl *control)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	return control->priv->uih;
}

/**
 * bonobo_control_set_property_bag:
 * @control: A #BonoboControl object.
 * @pb: A #BonoboPropertyBag.
 *
 * Binds @pb to @control.  When a remote object queries @control
 * for its property bag, @pb will be used in the responses.
 */
void
bonobo_control_set_property_bag (BonoboControl *control, BonoboPropertyBag *pb)
{
	g_return_if_fail (BONOBO_IS_CONTROL (control));
	g_return_if_fail (BONOBO_IS_PROPERTY_BAG (pb));

	control->priv->propbag = pb;
}

/**
 * bonobo_control_get_property_bag:
 * @control: A #BonoboControl whose PropertyBag has already been set.
 *
 * Returns: The #BonoboPropertyBag bound to @control.
 */
BonoboPropertyBag *
bonobo_control_get_property_bag (BonoboControl *control)
{
	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	return control->priv->propbag;
}

/**
 * bonobo_control_get_ambient_properties:
 * @control: A #BonoboControl which is bound to a remote
 * #BonoboControlFrame.
 *
 * Returns: A #Bonobo_PropertyBag bound to the bag of ambient
 * properties associated with this #Control's #ControlFrame.
 */
Bonobo_PropertyBag
bonobo_control_get_ambient_properties (BonoboControl     *control,
				       CORBA_Environment *ev)
{
	Bonobo_ControlFrame control_frame;
	Bonobo_PropertyBag pbag;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_val_if_fail (BONOBO_IS_CONTROL (control), NULL);

	control_frame = control->priv->control_frame;

	if (control_frame == CORBA_OBJECT_NIL)
		return NULL;

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	pbag = Bonobo_ControlFrame_get_ambient_properties (
		control_frame, real_ev);

	if (real_ev->_major != CORBA_NO_EXCEPTION) {
		if (!ev)
			CORBA_exception_free (&tmp_ev);
		pbag = CORBA_OBJECT_NIL;
	}

	return pbag;
}

/**
 * bonobo_control_get_remote_ui_handler:

 * @control: A BonoboControl object which is associated with a remote
 * ControlFrame.
 *
 * Returns: The Bonobo_UIContainer CORBA server for the remote BonoboControlFrame.
 */
Bonobo_Unknown
bonobo_control_get_remote_ui_handler (BonoboControl *control)
{
	CORBA_Environment  ev;
	Bonobo_UIContainer uih;

	g_return_val_if_fail (BONOBO_IS_CONTROL (control), CORBA_OBJECT_NIL);

	g_return_val_if_fail (control->priv->control_frame != CORBA_OBJECT_NIL,
			      CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	uih = Bonobo_ControlFrame_get_ui_handler (control->priv->control_frame, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (control), control->priv->control_frame, &ev);

	CORBA_exception_free (&ev);

	return uih;
}

/**
 * bonobo_control_activate_notify:
 * @control: A #BonoboControl object which is bound
 * to a remote ControlFrame.
 * @activated: Whether or not @control has been activated.
 *
 * Notifies the remote ControlFrame which is associated with
 * @control that @control has been activated/deactivated.
 */
void
bonobo_control_activate_notify (BonoboControl *control,
				gboolean      activated)
{
	CORBA_Environment ev;

	g_return_if_fail (BONOBO_IS_CONTROL (control));
	g_return_if_fail (control->priv->control_frame != CORBA_OBJECT_NIL);
	
	CORBA_exception_init (&ev);

	Bonobo_ControlFrame_activated (control->priv->control_frame, activated, &ev);

	bonobo_object_check_env (BONOBO_OBJECT (control), control->priv->control_frame, &ev);

	CORBA_exception_free (&ev);
}

static void
bonobo_control_class_init (BonoboControlClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;

	bonobo_control_parent_class = gtk_type_class (bonobo_object_get_type ());

	control_signals [SET_FRAME] =
                gtk_signal_new ("set_frame",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboControlClass, set_frame),
                                gtk_marshal_NONE__NONE,
                                GTK_TYPE_NONE, 0);

	control_signals [ACTIVATE] =
                gtk_signal_new ("activate",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboControlClass, activate),
                                gtk_marshal_NONE__BOOL,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	control_signals [UNDO_LAST_OPERATION] =
                gtk_signal_new ("undo_last_operation",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboControlClass, undo_last_operation),
                                gtk_marshal_NONE__NONE,
                                GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, control_signals, LAST_SIGNAL);

	object_class->destroy = bonobo_control_destroy;
	object_class->finalize = bonobo_control_finalize;

	init_control_corba_class ();
}

static void
bonobo_control_init (BonoboControl *control)
{
	control->priv = g_new0 (BonoboControlPrivate, 1);

	control->priv->control_frame = CORBA_OBJECT_NIL;
}

/**
 * bonobo_control_get_type:
 *
 * Returns: The GtkType corresponding to the BonoboControl class.
 */
GtkType
bonobo_control_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboControl",
			sizeof (BonoboControl),
			sizeof (BonoboControlClass),
			(GtkClassInitFunc) bonobo_control_class_init,
			(GtkObjectInitFunc) bonobo_control_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

void
bonobo_control_set_property (BonoboControl       *control,
			     const char          *first_prop,
			     ...)
{
	Bonobo_PropertyBag  bag;
	char               *err;
	CORBA_Environment   ev;
	va_list             args;

	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));

	va_start (args, first_prop);

	CORBA_exception_init (&ev);

	bag = bonobo_object_corba_objref (BONOBO_OBJECT (control->priv->propbag));

	if ((err = bonobo_property_bag_client_setv (bag, &ev, first_prop, args)))
		g_warning ("Error '%s'", err);

	CORBA_exception_free (&ev);

	va_end (args);
}

void
bonobo_control_get_property (BonoboControl       *control,
			     const char          *first_prop,
			     ...)
{
	Bonobo_PropertyBag  bag;
	char               *err;
	CORBA_Environment   ev;
	va_list             args;

	g_return_if_fail (first_prop != NULL);
	g_return_if_fail (BONOBO_IS_CONTROL (control));

	va_start (args, first_prop);

	CORBA_exception_init (&ev);

	bag = bonobo_object_corba_objref (BONOBO_OBJECT (control->priv->propbag));

	if ((err = bonobo_property_bag_client_getv (bag, &ev, first_prop, args)))
		g_warning ("Error '%s'", err);

	CORBA_exception_free (&ev);

	va_end (args);
}
