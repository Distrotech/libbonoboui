/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME view object
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@gnome-support.com)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-view.h>
#include <gdk/gdkprivate.h>

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_view_parent_class;

/* The entry point vectors for the server we provide */
static POA_GNOME_View__epv gnome_view_epv;
static POA_GNOME_View__vepv gnome_view_vepv;

enum {
	VIEW_ACTIVATE,
	DO_VERB,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL];

static void
impl_GNOME_View_activate (PortableServer_Servant servant,
			  CORBA_boolean activated,
			  CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (view), view_signals [VIEW_ACTIVATE], (gboolean) activated);
}

static void
impl_GNOME_View_do_verb (PortableServer_Servant servant,
			 CORBA_char *verb_name,
			 CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_object_from_servant (servant));
	GnomeViewVerbFunc callback;

	/*
	 * Always emit a signal when a verb is executed.
	 */
	gtk_signal_emit (
		GTK_OBJECT (view),
		view_signals [DO_VERB],
		(gchar *) verb_name);

	/*
	 * The user may have registered a callback for this particular
	 * verb.  If so, dispatch to that callback.
	 */
	callback = g_hash_table_lookup (view->verb_callbacks, verb_name);
	if (callback != NULL) {
		void *user_data;

		user_data = g_hash_table_lookup (view->verb_callback_closures, verb_name);

		(*callback) (view, (const char *) verb_name, user_data);
	}
}

static void
impl_GNOME_View_size_allocate (PortableServer_Servant servant,
			       const CORBA_short width,
			       const CORBA_short height,
			       CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_object_from_servant (servant));
	GtkAllocation allocation;

	if (view->plug == NULL)
		return;

	allocation.x = view->plug->allocation.x;
	allocation.y = view->plug->allocation.y;

	allocation.width = width;
	allocation.height = height;
	
	gtk_widget_size_allocate (view->plug, &allocation);
}

static void
impl_GNOME_View_set_window (PortableServer_Servant servant, GNOME_View_windowid id, CORBA_Environment *ev)
{
	GnomeView *view = GNOME_VIEW (gnome_object_from_servant (servant));

	view->plug = gtk_plug_new (id);

	gtk_widget_show_all (view->plug);

	gtk_container_add (GTK_CONTAINER (view->plug), view->widget);
}

/**
 * gnome_view_corba_object_create:
 * @object: the GtkObject that will wrap the CORBA object
 *
 * Creates and activates the CORBA object that is wrapped by the
 * @object GnomeObject.
 *
 * Returns: An activated object reference to the created object
 * or %CORBA_OBJECT_NIL in case of failure.
 */
GNOME_View
gnome_view_corba_object_create (GnomeObject *object)
{
	POA_GNOME_View *servant;
	
	servant = (POA_GNOME_View *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_view_vepv;

	POA_GNOME_View__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return (GNOME_View) gnome_object_activate_servant (object, servant);
	
}

/**
 * gnome_view_construct:
 * @view: The GnomeView object to be initialized.
 * @corba_view: The CORBA GNOME_View interface for the new GnomeView object.
 * @widget: A widget which contains the view and which will be passed
 * to the container process's ViewFrame object.
 *
 * Returns: the intialized GnomeView object.
 */
GnomeView *
gnome_view_construct (GnomeView *view, GNOME_View corba_view, GtkWidget *widget)
{
	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_VIEW (view), NULL);
	g_return_val_if_fail (corba_view != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	gnome_object_construct (GNOME_OBJECT (view), corba_view);
	
	view->widget = widget;
	gtk_object_ref (GTK_OBJECT (view->widget));

	view->verb_callbacks = g_hash_table_new (g_str_hash, g_str_equal);
	view->verb_callback_closures = g_hash_table_new (g_str_hash, g_str_equal);

	return view;
}

/**
 * gnome_view_new:
 * @widget: a GTK widget which contains the view and which will be
 * passed to the container process.
 *
 * This function creates a new GnomeView object for @widget.
 *
 * Returns: a GnomeView object that implements the GNOME::View CORBA
 * service that will transfer the @widget to the container process.
 */
GnomeView *
gnome_view_new (GtkWidget *widget)
{
	GnomeView *view;
	GNOME_View corba_view;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	view = gtk_type_new (gnome_view_get_type ());

	corba_view = gnome_view_corba_object_create (GNOME_OBJECT (view));
	if (corba_view == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (view));
		return NULL;
	}
	

	return gnome_view_construct (view, corba_view, widget);
}

static void
gnome_view_destroy (GtkObject *object)
{
	GnomeView *view = GNOME_VIEW (object);

	if (view->widget)
		gtk_object_unref (GTK_OBJECT (view->widget));

	GTK_OBJECT_CLASS (gnome_view_parent_class)->destroy (object);
}

static void
init_view_corba_class (void)
{
	/* The entry point vectors for this GNOME::View class */
	gnome_view_epv.size_allocate = impl_GNOME_View_size_allocate;
	gnome_view_epv.set_window = impl_GNOME_View_set_window;
	gnome_view_epv.do_verb = impl_GNOME_View_do_verb;
	gnome_view_epv.activate = impl_GNOME_View_activate;

	/* Setup the vector of epvs */
	gnome_view_vepv.GNOME_Unknown_epv = &gnome_object_epv;
	gnome_view_vepv.GNOME_View_epv = &gnome_view_epv;
}

static void
gnome_view_class_init (GnomeViewClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_view_parent_class = gtk_type_class (gnome_object_get_type ());

	view_signals [VIEW_ACTIVATE] =
                gtk_signal_new ("view_activate",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (GnomeViewClass, view_activate), 
                                gtk_marshal_NONE__BOOL,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_BOOL);

	view_signals [DO_VERB] =
                gtk_signal_new ("do_verb",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (GnomeViewClass, do_verb), 
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, view_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_view_destroy;

	init_view_corba_class ();
}

static void
gnome_view_init (GnomeView *view)
{
}

/**
 * gnome_view_get_type:
 *
 * Returns: The GtkType corresponding to the GnomeView class.
 */
GtkType
gnome_view_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/View:1.0",
			sizeof (GnomeView),
			sizeof (GnomeViewClass),
			(GtkClassInitFunc) gnome_view_class_init,
			(GtkObjectInitFunc) gnome_view_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_view_set_embeddable:
 * @view: A GnomeView object.
 * @embeddable: The GnomeEmbeddable object for which @view is a view.
 *
 * This function associates @view with the specified GnomeEmbeddabe
 * object, @embeddable.
 */
void
gnome_view_set_embeddable (GnomeView *view, GnomeEmbeddable *embeddable)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (GNOME_IS_VIEW (view));
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));

	if (view->embeddable != NULL)
		gtk_object_unref (GTK_OBJECT (view->embeddable));

	view->embeddable = embeddable;
	gtk_object_ref (GTK_OBJECT (view->embeddable));
}

/**
 * gnome_view_get_embeddable:
 * @view: A GnomeView object.
 *
 * Returns: The GnomeEmbeddable object for which @view is a GnomeView.
 */
GnomeEmbeddable *
gnome_view_get_embeddable (GnomeView *view)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (GNOME_IS_VIEW (view));

	return view->embeddable;
}

/**
 * gnome_view_set_view_frame:
 * @view: A GnomeView object.
 * @view_frame: A CORBA interface for the ViewFrame which contains this View.
 *
 * Sets the ViewFrame for @view to @view_frame.
 */
void
gnome_view_set_view_frame (GnomeView *view, GNOME_ViewFrame view_frame)
{
	CORBA_Environment ev;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GNOME_IS_VIEW (view));

	CORBA_exception_init (&ev);

	GNOME_Unknown_ref (view_frame, &ev);

	view->view_frame = CORBA_Object_duplicate (view_frame, &ev);

	CORBA_exception_free (&ev);
}

/**
 * gnome_view_get_view_frame:
 * @view: A GnomeView object whose GNOME_ViewFrame CORBA interface is
 * being retrieved.
 *
 * Returns: The GNOME_ViewFrame CORBA object associated with @view.a
 */
GNOME_ViewFrame
gnome_view_get_view_frame (GnomeView *view)

{
	g_return_val_if_fail (view != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_VIEW (view), CORBA_OBJECT_NIL);

	return view->view_frame;
}

/**
 * gnome_view_get_ui_handler:
 * @view: A GnomeView object which is bound to a remote GnomeViewFrame.
 *
 * Returns: The GNOME_UIHandler CORBA server for the remote GnomeViewFrame.
 */
GNOME_UIHandler
gnome_view_get_ui_handler (GnomeView *view)
{
	CORBA_Environment ev;
	GNOME_UIHandler uih;

	g_return_val_if_fail (view != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_VIEW (view), CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	uih = GNOME_ViewFrame_get_ui_handler (view->view_frame, &ev);

	CORBA_exception_free (&ev);

	return uih;
}


/**
 * gnome_view_register_verb:
 * @view: A GnomeView object.
 * @verb_name: The name of the verb to register.
 * @callback: A function to call when @verb_name is executed on @view.
 * @user_data: A closure to pass to @callback when it is invoked.
 *
 * Registers a verb called @verb_name against @view.  When @verb_name
 * is executed, the View will dispatch to @callback.
 */
void
gnome_view_register_verb (GnomeView *view, const char *verb_name,
			  GnomeViewVerbFunc callback, gpointer user_data)
{
	char *key;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GNOME_IS_VIEW (view));
	g_return_if_fail (verb_name != NULL);

	key = g_strdup (verb_name);

	g_hash_table_insert (view->verb_callbacks, key, callback);
	g_hash_table_insert (view->verb_callback_closures, key, user_data);
}

/**
 * gnome_view_unregister_verb:
 * @view: A GnomeView object.
 * @verb_name: The name of a verb to be unregistered.
 *
 * Unregisters the verb called @verb_name from @view.
 */
void
gnome_view_unregister_verb (GnomeView *view, const char *verb_name)
{
	gchar *original_key;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GNOME_IS_VIEW (view));
	g_return_if_fail (verb_name != NULL);

	if (! g_hash_table_lookup_extended (view->verb_callbacks, verb_name,
					    (gpointer *) &original_key, NULL))
		return;
	g_hash_table_remove (view->verb_callbacks, verb_name);
	g_hash_table_remove (view->verb_callback_closures, verb_name);
	g_free (original_key);
}

