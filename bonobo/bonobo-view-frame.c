/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME view frame object.
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
#include <bonobo/gnome-view.h>
#include <bonobo/gnome-view-frame.h>
#include <gdk/gdkprivate.h>
#include <libgnomeui/gnome-canvas.h>

enum {
	VIEW_ACTIVATED,
	UNDO_LAST_OPERATION,
	USER_ACTIVATE,
	USER_CONTEXT,
	LAST_SIGNAL
};

static guint view_frame_signals [LAST_SIGNAL];

/* Parent object class in GTK hierarchy */
static GnomeControlFrameClass *gnome_view_frame_parent_class;

/* The entry point vectors for the server we provide */
POA_GNOME_ViewFrame__epv gnome_view_frame_epv;
POA_GNOME_ViewFrame__vepv gnome_view_frame_vepv;

struct _GnomeViewFramePrivate {
	GnomeWrapper    *wrapper; 
	GnomeClientSite *client_site;
	GNOME_View       view;
};

static GNOME_ClientSite
impl_GNOME_ViewFrame_get_ui_handler (PortableServer_Servant servant,
				     CORBA_Environment *ev)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (gnome_object_from_servant (servant));

	if (view_frame->uih == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;
	
	return CORBA_Object_duplicate (
		gnome_object_corba_objref (GNOME_OBJECT (view_frame->uih)), ev);
}

static GNOME_ClientSite
impl_GNOME_ViewFrame_get_client_site (PortableServer_Servant servant,
				      CORBA_Environment *ev)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (gnome_object_from_servant (servant));

	return CORBA_Object_duplicate (
		gnome_object_corba_objref (GNOME_OBJECT (view_frame->priv->client_site)), ev);
}

static void
impl_GNOME_ViewFrame_view_activated (PortableServer_Servant servant,
				     const CORBA_boolean state,
				     CORBA_Environment *ev)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (gnome_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (view_frame),
			 view_frame_signals [VIEW_ACTIVATED], state);
}

static void
impl_GNOME_ViewFrame_view_deactivate_and_undo (PortableServer_Servant servant,
					       CORBA_Environment *ev)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (gnome_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (view_frame),
			 view_frame_signals [VIEW_ACTIVATED], FALSE);

	gtk_signal_emit (GTK_OBJECT (view_frame),
			 view_frame_signals [UNDO_LAST_OPERATION]);
}

static CORBA_Object
create_gnome_view_frame (GnomeObject *object)
{
	POA_GNOME_ViewFrame *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_ViewFrame *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_view_frame_vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_ViewFrame__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return gnome_object_activate_servant (object, servant);
}

/**
 * gnome_view_frame_construct:
 * @view_frame: The GnomeViewFrame object to be initialized.
 * @corba_view_frame: A CORBA object for the GNOME_ViewFrame interface.
 * @wrapper: A GnomeWrapper widget which the new ViewFrame will use to cover its enclosed View.
 * @client_site: the client site to which the newly-created ViewFrame will belong.
 *
 * Initializes @view_frame with the parameters.
 *
 * Returns: the initialized GnomeViewFrame object @view_frame that implements the
 * GNOME::ViewFrame CORBA service.
 */
GnomeViewFrame *
gnome_view_frame_construct (GnomeViewFrame *view_frame,
			    GNOME_ViewFrame corba_view_frame,
			    GnomeWrapper   *wrapper,
			    GnomeClientSite *client_site)
{
	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);
	g_return_val_if_fail (wrapper != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_WRAPPER (wrapper), NULL);
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);

	gnome_control_frame_construct (GNOME_CONTROL_FRAME (view_frame), corba_view_frame);
	
	view_frame->priv->client_site = client_site;
	view_frame->priv->wrapper = wrapper;

	return view_frame;
}

static gboolean
gnome_view_frame_wrapper_button_press_cb (GtkWidget *wrapper,
					  GdkEventButton *event,
					  gpointer data)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (data);

	/* Check for double click. */
	if (event->type == GDK_2BUTTON_PRESS)
		gtk_signal_emit (GTK_OBJECT (view_frame), view_frame_signals [USER_ACTIVATE]);

	/* Check for right click. */
	else if (event->type == GDK_BUTTON_PRESS &&
		 event->button == 3)
		gtk_signal_emit (GTK_OBJECT (view_frame), view_frame_signals [USER_CONTEXT]);
		
	return FALSE;
} 

/**
 * gnome_view_frame_new:
 * @client_site: the client site to which the newly-created ViewFrame will belong.
 *
 * Returns: GnomeViewFrame object that implements the
 * GNOME::ViewFrame CORBA service.
 */
GnomeViewFrame *
gnome_view_frame_new (GnomeClientSite *client_site)
{
	GNOME_ViewFrame corba_view_frame;
	GnomeViewFrame *view_frame;
	GnomeWrapper   *wrapper;
	
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);

	view_frame = gtk_type_new (GNOME_VIEW_FRAME_TYPE);

	wrapper = GNOME_WRAPPER (gnome_wrapper_new ());
	if (wrapper == NULL) {
	       gtk_object_unref (GTK_OBJECT (view_frame));
	       return NULL;
	}

	/*
	 * Connect a signal handler so that we can catch double clicks
	 * on the wrapper.
	 */
	gtk_signal_connect (GTK_OBJECT (wrapper), "button_press_event",
			    GTK_SIGNAL_FUNC (gnome_view_frame_wrapper_button_press_cb),
			    view_frame);

	corba_view_frame = create_gnome_view_frame (GNOME_OBJECT (view_frame));
	if (corba_view_frame == CORBA_OBJECT_NIL) {
		gtk_object_destroy (GTK_OBJECT (view_frame));
		return NULL;
	}
	

	return gnome_view_frame_construct (view_frame, corba_view_frame, wrapper, client_site);
}

static void
gnome_view_frame_destroy (GtkObject *object)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (object);

	if (view_frame->priv->view != CORBA_OBJECT_NIL){
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
                GNOME_Unknown_unref (view_frame->priv->view, &ev);
		CORBA_Object_release (view_frame->priv->view, &ev);
		CORBA_exception_free (&ev);
	}
	
	GTK_OBJECT_CLASS (gnome_view_frame_parent_class)->destroy (object);

	gtk_object_destroy (GTK_OBJECT (view_frame->priv->wrapper));
	g_free (view_frame->priv);

}

static void
init_view_frame_corba_class (void)
{
	/* The entry point vectors for this GNOME::View class */
	gnome_view_frame_epv.get_client_site = impl_GNOME_ViewFrame_get_client_site;
	gnome_view_frame_epv.get_ui_handler = impl_GNOME_ViewFrame_get_ui_handler;
	gnome_view_frame_epv.view_activated = impl_GNOME_ViewFrame_view_activated;
	gnome_view_frame_epv.deactivate_and_undo = impl_GNOME_ViewFrame_view_deactivate_and_undo;
	
	/* Setup the vector of epvs */
	gnome_view_frame_vepv.GNOME_Unknown_epv = &gnome_object_epv;
	gnome_view_frame_vepv.GNOME_ControlFrame_epv = &gnome_control_frame_epv;
	gnome_view_frame_vepv.GNOME_ViewFrame_epv = &gnome_view_frame_epv;
}

static void
gnome_view_frame_activated (GnomeViewFrame *view_frame, gboolean state)
{
	
}

static void
gnome_view_frame_class_init (GnomeViewFrameClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_view_frame_parent_class = gtk_type_class (GNOME_CONTROL_FRAME_TYPE);

	view_frame_signals [VIEW_ACTIVATED] =
		gtk_signal_new ("view_activated",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeViewFrameClass, view_activated),
				gtk_marshal_NONE__BOOL,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	view_frame_signals [UNDO_LAST_OPERATION] =
		gtk_signal_new ("undo_last_operation",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeViewFrameClass, undo_last_operation),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	view_frame_signals [USER_ACTIVATE] =
		gtk_signal_new ("user_activate",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeViewFrameClass, user_activate),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	view_frame_signals [USER_CONTEXT] =
		gtk_signal_new ("user_context",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeViewFrameClass, user_context),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (
		object_class,
		view_frame_signals,
		LAST_SIGNAL);

	object_class->destroy = gnome_view_frame_destroy;

	class->view_activated = gnome_view_frame_activated;

	init_view_frame_corba_class ();
}

static void
gnome_view_frame_init (GnomeObject *object)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (object);

	view_frame->priv = g_new0 (GnomeViewFramePrivate, 1);
}

/**
 * gnome_view_frame_get_type:
 *
 * Returns: The GtkType for the GnomeViewFrame class.
 */
GtkType
gnome_view_frame_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"GnomeViewFrame",
			sizeof (GnomeViewFrame),
			sizeof (GnomeViewFrameClass),
			(GtkClassInitFunc) gnome_view_frame_class_init,
			(GtkObjectInitFunc) gnome_view_frame_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_control_frame_get_type (), &info);
	}

	return type;
}

/**
 * gnome_view_frame_bind_to_view:
 * @view_frame: A GnomeViewFrame object.
 * @view: The CORBA object for the GnomeView embedded
 * in this ViewFrame.
 *
 * Associates @view with this @view_frame.
 */
void
gnome_view_frame_bind_to_view (GnomeViewFrame *view_frame, GNOME_View view)
{
	CORBA_Environment ev;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));

	CORBA_exception_init (&ev);
	gnome_control_frame_bind_to_control (
		GNOME_CONTROL_FRAME (view_frame),
		(GNOME_Control) view);
	view_frame->priv->view = CORBA_Object_duplicate (view, &ev);

	CORBA_exception_free (&ev);
}

/**
 * gnome_view_frame_get_view:
 * @view_frame: A GnomeViewFrame object.
 * @view: The CORBA object for the GnomeView embedded
 * in this ViewFrame.
 *
 * Associates @view with this @view_frame.
 */
GNOME_View
gnome_view_frame_get_view (GnomeViewFrame *view_frame)
{
	g_return_val_if_fail (view_frame != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), CORBA_OBJECT_NIL);

	return view_frame->priv->view;
}

/**
 * gnome_view_frame_set_covered:
 * @view_frame: A GnomeViewFrame object whose embedded View should be
 * either covered or uncovered.
 * @covered: %TRUE if the View should be covered.  %FALSE if it should
 * be uncovered.
 *
 * This function either covers or uncovers the View embedded in a
 * GnomeViewFrame.  If the View is covered, then the embedded widgets
 * will receive no Gtk events, such as mouse movements, keypresses,
 * and exposures.  When the View is uncovered, all events pass through
 * to the GnomeView's widgets normally.
 */
void
gnome_view_frame_set_covered (GnomeViewFrame *view_frame, gboolean covered)
{
	GtkWidget *wrapper;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));

	wrapper = gnome_view_frame_get_wrapper (view_frame);
	gnome_wrapper_set_covered (GNOME_WRAPPER (wrapper), covered);
}

/**
 * gnome_view_frame_view_activate:
 * @view_frame: The GnomeViewFrame object whose view should be
 * activated.
 *
 * Activates the GnomeView embedded in @view_frame by calling the
 * activate() #GNOME_View interface method on it.
 */
void
gnome_view_frame_view_activate (GnomeViewFrame *view_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));

	/*
	 * Check that this ViewFrame actually has a View associated
	 * with it.
	 */
	g_return_if_fail (view_frame->priv->view != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	GNOME_View_activate (view_frame->priv->view, TRUE, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (
			GNOME_OBJECT (view_frame),
			(CORBA_Object) view_frame->priv->view, &ev);
	}

	CORBA_exception_free (&ev);
}


/**
 * gnome_view_frame_view_deactivate:
 * @view_frame: The GnomeViewFrame object whose view should be
 * deactivated.
 *
 * Deactivates the GnomeView embedded in @view_frame by calling a the
 * activate() CORBA method on it with the parameter %FALSE.
 */
void
gnome_view_frame_view_deactivate (GnomeViewFrame *view_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));

	/*
	 * Check that this ViewFrame actually has a View associated
	 * with it.
	 */
	g_return_if_fail (view_frame->priv->view != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	GNOME_View_activate (view_frame->priv->view, FALSE, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (
			GNOME_OBJECT (view_frame),
			(CORBA_Object) view_frame->priv->view, &ev);
	}

	CORBA_exception_free (&ev);
}

/**
 * gnome_view_frame_view_do_verb:
 * @view_frame: A GnomeViewFrame object which has an associated GnomeView.
 * @verb_name: The name of the verb to perform on @view_frame's GnomeView.
 *
 * Performs the verb specified by @verb_name on the remote GnomeView
 * object associated with @view_frame.
 */
void
gnome_view_frame_view_do_verb (GnomeViewFrame *view_frame,
			       char *verb_name)
{
	CORBA_Environment ev;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));
	g_return_if_fail (verb_name != NULL);
	g_return_if_fail (view_frame->priv->view != CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);
	GNOME_View_do_verb (view_frame->priv->view, verb_name, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (
			GNOME_OBJECT (view_frame),
			(CORBA_Object) view_frame->priv->view, &ev);
	}
	CORBA_exception_free (&ev);
}

/**
 * gnome_view_frame_get_wrapper:
 * @view_frame: A GnomeViewFrame object.
 *
 * Returns: The GnomeWrapper widget associated with this ViewFrame.
 */
GtkWidget *
gnome_view_frame_get_wrapper (GnomeViewFrame *view_frame)
{
	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);

	return GTK_WIDGET (view_frame->priv->wrapper);
}

/**
 * gnome_view_frame_set_ui_handler:
 * @view_frame: A GnomeViewFrame object.
 * @uih: A GnomeUIHandler object to be associated with this ViewFrame.
 *
 * Sets the GnomeUIHandler object for this ViewFrame.  When the
 * ViewFrame's View requests its container's UIHandler interface, the
 * ViewFrame will pass it the UIHandler specified here.  See also
 * gnome_view_frame_get_ui_handler().
 */
void
gnome_view_frame_set_ui_handler (GnomeViewFrame *view_frame, GnomeUIHandler *uih)
{
	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	view_frame->uih = uih;
}

/**
 * gnome_view_frame_get_ui_handler:
 * @view_frame: A GnomeViewFrame object.
 *
 * Returns: The GNOMEUIHandler associated with this ViewFrame.  See
 * also gnome_view_frame_set_ui_handler().
 */
GnomeUIHandler *
gnome_view_frame_get_ui_handler (GnomeViewFrame *view_frame)
{
	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);

	return view_frame->uih;
}

/**
 * gnome_view_frame_size_request:
 * @view_frame: A GnomeViewFrame object.
 * @desired_width: pointer to an integer, where the desired width of the View is stored
 * @desired_height: pointer to an integer, where the desired height of the View is stored
 *
 * Returns: The default desired_width and desired_height the component
 * wants to use.
 */
void
gnome_view_frame_size_request (GnomeViewFrame *view_frame, int *desired_width, int *desired_height)
{
	CORBA_short dw, dh;
	CORBA_Environment ev;
	
	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));
	g_return_if_fail (desired_height != NULL);
	g_return_if_fail (desired_width  != NULL);

	dw = 0;
	dh = 0;
	
	CORBA_exception_init (&ev);
	GNOME_View_size_query (view_frame->priv->view, &dw, &dh, &ev);
	if (ev._major == CORBA_NO_EXCEPTION){
		*desired_width = dw;
		*desired_height = dh;
	} else {
		if (ev._major != CORBA_NO_EXCEPTION) {
			gnome_object_check_env (
				GNOME_OBJECT (view_frame),
				(CORBA_Object) view_frame->priv->view, &ev);
		}
	}
	CORBA_exception_free (&ev);
}

/**
 * gnome_view_frame_set_zoom_factor:
 * @view_frame: A GnomeViewFrame object.
 * @zoom: a zoom factor.  1.0 means one-to-one mapping.
 *
 * Requests the associated view to change its zoom factor the the value in @zoom.
 */
void
gnome_view_frame_set_zoom_factor (GnomeViewFrame *view_frame, double zoom)
{
	CORBA_Environment ev;

	g_return_if_fail (view_frame != NULL);
	g_return_if_fail (GNOME_IS_VIEW_FRAME (view_frame));
	g_return_if_fail (zoom > 0.0);

	CORBA_exception_init (&ev);
	GNOME_View_set_zoom_factor (view_frame->priv->view, zoom, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (
			GNOME_OBJECT (view_frame),
			(CORBA_Object) view_frame->priv->view, &ev);
	}
	CORBA_exception_free (&ev);
}

static void
gnome_view_frame_verb_selected_cb (GnomeUIHandler *uih, void *user_data, char *path)
{
	GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (user_data);
	char *verb_name;

	g_assert (path != NULL);

	/*
	 * A verb was selected.  Extract the verb name from the menu
	 * item path.
	 */
	verb_name = path + 1;

	/*
	 * Now execute the verb on the remote View.
	 */
	gnome_view_frame_view_do_verb (view_frame, verb_name);

	/*
	 * Store the verb name.
	 */
	gtk_object_set_data (GTK_OBJECT (view_frame), "view_frame_executed_verb_name", g_strdup (verb_name));
}

/**
 * gnome_view_frame_popup_verbs:
 * @view_frame: A GnomeViewFrame object which is bound to a remote
 * GnomeView.
 *
 * This function creates a popup menu containing the available verbs
 * for the remote GnomeEmbeddable.  When the user selects a verb in
 * the menu, the menu is destroyed, and the verb is executed on the
 * view to which @view_frame is bound.  This function is meant to act
 * as a convenience, to save people the trouble of having to
 * reimplement this functionality over and over again.
 *
 * Returns: The name of the verb which the user selected, or %NULL if
 * no verb was selected.
 */
char *
gnome_view_frame_popup_verbs (GnomeViewFrame *view_frame)
{
	GnomeUIHandler *popup;
	GList *verbs, *l;
	char *verb;

	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);
	g_return_val_if_fail (view_frame->priv->view != CORBA_OBJECT_NIL, NULL);

	/*
	 * First get the list of available verbs from the remote
	 * GnomeEmbeddable.
	 */
	verbs = gnome_client_site_get_verbs (view_frame->priv->client_site);

	/*
	 * Now build a menu.
	 */
	popup = gnome_ui_handler_new ();
	gnome_ui_handler_create_popup_menu (popup);

	for (l = verbs; l != NULL; l = l->next) {
		GnomeVerb *verb = (GnomeVerb *) l->data;
		char *path;

		path = g_strconcat ("/", verb->name, NULL);
		gnome_ui_handler_menu_new_item (popup, path,
						verb->label, verb->hint,
						-1,
						GNOME_UI_HANDLER_PIXMAP_NONE, NULL,
						0, (GdkModifierType) 0,
						gnome_view_frame_verb_selected_cb,
						view_frame);

		g_free (path);
	}

	/*
	 * Pop up the menu.
	 */
	gnome_ui_handler_do_popup_menu (popup);

	/*
	 * Destroy it.
	 */
	gnome_object_unref (GNOME_OBJECT (popup));

	/*
	 * Grab the name of the executed verb.
	 */
	verb = gtk_object_get_data (GTK_OBJECT (view_frame), "view_frame_executed_verb_name");
	gtk_object_remove_data (GTK_OBJECT (view_frame), "view_frame_executed_verb_name");

	return verb;
}

/**
 * gnome_view_frame_get_client_site:
 * @view_frame: The view frame
 *
 * Returns the GnomeClientSite associated with this view frame
 */
GnomeClientSite *
gnome_view_frame_get_client_site (GnomeViewFrame *view_frame)
{
	g_return_val_if_fail (view_frame != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW_FRAME (view_frame), NULL);

	return view_frame->priv->client_site;
}

