/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME view object
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkplug.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-view.h>
#include <gdk/gdkprivate.h>

/* Parent object class in GTK hierarchy */
static BonoboControlClass *bonobo_view_parent_class;

/* The entry point vectors for the server we provide */
POA_Bonobo_View__epv    bonobo_view_epv;
POA_Bonobo_Control__epv bonobo_view_overridden_control_epv;
POA_Bonobo_View__vepv   bonobo_view_vepv;

enum {
	VIEW_UNDO_LAST_OPERATION,
	DO_VERB,
	SET_ZOOM_FACTOR,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL];

typedef void (*GnomeSignal_NONE__DOUBLE) (GtkObject *object, double arg1, gpointer user_data);

struct _BonoboViewPrivate {
	GHashTable *verb_callbacks;
	GHashTable *verb_callback_closures;
};

static void
impl_Bonobo_View_do_verb (PortableServer_Servant servant,
			 const CORBA_char      *verb_name,
			 CORBA_Environment     *ev)
{
#ifdef USE_UI_HANDLER
	BonoboView *view = BONOBO_VIEW (bonobo_object_from_servant (servant));

	bonobo_view_execute_verb (view, verb_name);
#else
	g_warning ("Verbs have jumped interface");
#endif
}

static void
impl_Bonobo_View_set_zoom_factor (PortableServer_Servant servant,
				 const CORBA_double zoom,
				 CORBA_Environment *ev)
{
	BonoboView *view = BONOBO_VIEW (bonobo_object_from_servant (servant));

	gtk_signal_emit (GTK_OBJECT (view),
			 view_signals [SET_ZOOM_FACTOR], zoom);
}

/**
 * bonobo_view_corba_object_create:
 * @object: the GtkObject that will wrap the CORBA object
 *
 * Creates and activates the CORBA object that is wrapped by the
 * @object BonoboObject.
 *
 * Returns: An activated object reference to the created object
 * or %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_View
bonobo_view_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_View *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_View *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_view_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_View__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){

		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (Bonobo_View) bonobo_object_activate_servant (object, servant);
}

#if 0
/**
 * bonobo_view_activate:
 * @view: 
 * @activate: 
 * 
 *   A basic default handler so we can at least
 * get focus in a development component; 95% of these
 * will override this to merge menus for them.
 **/
static void
bonobo_view_activate (BonoboView *view, gboolean activate, gpointer user_data)
{
	bonobo_view_activate_notify (view, activate);
}
#endif

/**
 * bonobo_view_construct:
 * @view: The BonoboView object to be initialized.
 * @corba_view: The CORBA Bonobo_View interface for the new BonoboView object.
 * @widget: A GtkWidget contains the view and * will be passed to the container
 * process's ViewFrame object.
 * 
 * @item_creator might be NULL for widget-based views.
 *
 * Returns: the intialized BonoboView object.
 */
BonoboView *
bonobo_view_construct (BonoboView *view, Bonobo_View corba_view, GtkWidget *widget)
{
	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), NULL);
	g_return_val_if_fail (corba_view != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
	
	bonobo_control_construct (BONOBO_CONTROL (view), corba_view, widget);

	view->priv->verb_callbacks = g_hash_table_new (g_str_hash, g_str_equal);
	view->priv->verb_callback_closures = g_hash_table_new (g_str_hash, g_str_equal);

/*	gtk_signal_connect (GTK_OBJECT (view), "view_activate",
			    GTK_SIGNAL_FUNC (bonobo_view_activate),
			    NULL);*/
	return view;
}

/**
 * bonobo_view_new:
 * @widget: a GTK widget that contains the view and will be passed to the
 * container process.
 *
 * This function creates a new BonoboView object for @widget
 *
 * Returns: a BonoboView object that implements the Bonobo::View CORBA
 * service that will transfer the @widget to the container process.
 */
BonoboView *
bonobo_view_new (GtkWidget *widget)
{
	BonoboView *view;
	Bonobo_View corba_view;
	
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	view = gtk_type_new (bonobo_view_get_type ());

	corba_view = bonobo_view_corba_object_create (BONOBO_OBJECT (view));
	if (corba_view == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (view));
		return NULL;
	}
	
	return bonobo_view_construct (view, corba_view, widget);
}

static gboolean
bonobo_view_destroy_remove_verb (gpointer key, gpointer value,
				gpointer user_data)
{
	g_free (key);

	return TRUE;
}

static void
bonobo_view_destroy (GtkObject *object)
{
	BonoboView *view;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (object));
	
	view = BONOBO_VIEW (object);

	/*
	 * Free up all the verbs associated with this View.
	 */
	g_hash_table_foreach_remove (view->priv->verb_callbacks,
				     bonobo_view_destroy_remove_verb, NULL);
	g_hash_table_destroy (view->priv->verb_callbacks);

	g_hash_table_destroy (view->priv->verb_callback_closures);

	g_free (view->priv);
	
	bonobo_object_unref (BONOBO_OBJECT (view->embeddable));

	GTK_OBJECT_CLASS (bonobo_view_parent_class)->destroy (object);
}

/**
 * bonobo_view_get_epv:
 */
POA_Bonobo_View__epv *
bonobo_view_get_epv (void)
{
	POA_Bonobo_View__epv *epv;

	epv = g_new0 (POA_Bonobo_View__epv, 1);

	epv->do_verb	     = impl_Bonobo_View_do_verb;
	epv->set_zoom_factor = impl_Bonobo_View_set_zoom_factor;

	return epv;
}

static void
init_view_corba_class (void)
{
	bonobo_view_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_view_vepv.Bonobo_Control_epv = bonobo_control_get_epv ();
	bonobo_view_vepv.Bonobo_View_epv    = bonobo_view_get_epv ();
}

static void 
gnome_marshal_NONE__DOUBLE (GtkObject * object,
			    GtkSignalFunc func,
			    gpointer func_data,
			    GtkArg * args)
{
	GnomeSignal_NONE__DOUBLE rfunc;
	rfunc = (GnomeSignal_NONE__DOUBLE) func;
	(*rfunc) (object,
		  GTK_VALUE_DOUBLE (args[0]),
		  func_data);
}

static void
bonobo_view_class_init (BonoboViewClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_view_parent_class = gtk_type_class (bonobo_control_get_type ());

	view_signals [DO_VERB] =
                gtk_signal_new ("do_verb",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboViewClass, do_verb), 
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	view_signals [SET_ZOOM_FACTOR] =
                gtk_signal_new ("set_zoom_factor",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (BonoboViewClass, set_zoom_factor), 
                                gnome_marshal_NONE__DOUBLE,
                                GTK_TYPE_NONE, 1,
				GTK_TYPE_DOUBLE);

	gtk_object_class_add_signals (object_class, view_signals, LAST_SIGNAL);

	object_class->destroy = bonobo_view_destroy;

	init_view_corba_class ();
}

static void
bonobo_view_init (BonoboView *view)
{
	view->priv = g_new0 (BonoboViewPrivate, 1);
}

/**
 * bonobo_view_get_type:
 *
 * Returns: The GtkType corresponding to the BonoboView class.
 */
GtkType
bonobo_view_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboView",
			sizeof (BonoboView),
			sizeof (BonoboViewClass),
			(GtkClassInitFunc) bonobo_view_class_init,
			(GtkObjectInitFunc) bonobo_view_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_control_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_view_set_embeddable:
 * @view: A BonoboView object.
 * @embeddable: The BonoboEmbeddable object for which @view is a view.
 *
 * This function associates @view with the specified GnomeEmbeddabe
 * object, @embeddable.
 */
void
bonobo_view_set_embeddable (BonoboView *view, BonoboEmbeddable *embeddable)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (view));
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));

	if (view->embeddable != NULL)
		bonobo_object_unref (BONOBO_OBJECT (view->embeddable));

	view->embeddable = embeddable;
	bonobo_object_ref (BONOBO_OBJECT (view->embeddable));
}

/**
 * bonobo_view_get_embeddable:
 * @view: A BonoboView object.
 *
 * Returns: The BonoboEmbeddable object for which @view is a BonoboView.
 */
BonoboEmbeddable *
bonobo_view_get_embeddable (BonoboView *view)
{
	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), NULL);

	return view->embeddable;
}

/**
 * bonobo_view_set_view_frame:
 * @view: A BonoboView object.
 * @view_frame: A CORBA interface for the ViewFrame which contains this View.
 *
 * Sets the ViewFrame for @view to @view_frame.
 */
void
bonobo_view_set_view_frame (BonoboView *view, Bonobo_ViewFrame view_frame)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (view));
	
	bonobo_control_set_control_frame (BONOBO_CONTROL (view), (Bonobo_ControlFrame) view_frame);

	view->view_frame = view_frame;
}

/**
 * bonobo_view_get_view_frame:
 * @view: A BonoboView object whose Bonobo_ViewFrame CORBA interface is
 * being retrieved.
 *
 * Returns: The Bonobo_ViewFrame CORBA object associated with @view, this is
 * a CORBA_object_duplicated object.  You need to CORBA_free it when you are
 * done with it.
 */
Bonobo_ViewFrame
bonobo_view_get_view_frame (BonoboView *view)

{
	g_return_val_if_fail (view != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), CORBA_OBJECT_NIL);
	
	return view->view_frame;
}

/**
 * bonobo_view_get_ui_handler:
 * @view: A BonoboView object for which a BonoboUIHandler has been created and set.
 *
 * Returns: The BonoboUIHandler which was associated with @view when it was created.
 */
BonoboUIHandler *
bonobo_view_get_ui_handler (BonoboView *view)
{
	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), NULL);

	return bonobo_control_get_ui_handler (BONOBO_CONTROL (view));
}

/**
 * bonobo_view_get_remote_ui_handler:
 * @view: A BonoboView object which is bound to a remote BonoboViewFrame.
 *
 * Returns: The Bonobo_UIHandler CORBA server for the remote BonoboViewFrame.
 */
Bonobo_UIHandler
bonobo_view_get_remote_ui_handler (BonoboView *view)
{
	g_return_val_if_fail (view != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), CORBA_OBJECT_NIL);

	return bonobo_control_get_remote_ui_handler (BONOBO_CONTROL (view));
}

/**
 * bonobo_view_activate_notify:
 * @view: A BonoboView object which is bound to a remote BonoboViewFrame..
 * @activate: %TRUE if the view is activated, %FALSE otherwise.
 *
 * This function notifies @view's remote ViewFrame that the activation
 * state of @view has changed.
 */
void
bonobo_view_activate_notify (BonoboView *view, gboolean activated)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (view));

	bonobo_control_activate_notify (BONOBO_CONTROL (view), activated);
}


/**
 * bonobo_view_register_verb:
 * @view: A BonoboView object.
 * @verb_name: The name of the verb to register.
 * @callback: A function to call when @verb_name is executed on @view.
 * @user_data: A closure to pass to @callback when it is invoked.
 *
 * Registers a verb called @verb_name against @view.  When @verb_name
 * is executed, the View will dispatch to @callback.
 */
void
bonobo_view_register_verb (BonoboView *view, const char *verb_name,
			  BonoboViewVerbFunc callback, gpointer user_data)
{
	char *key;

	g_return_if_fail (view != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (view));
	g_return_if_fail (verb_name != NULL);

	key = g_strdup (verb_name);

	g_hash_table_insert (view->priv->verb_callbacks, key, callback);
	g_hash_table_insert (view->priv->verb_callback_closures, key, user_data);
}

/**
 * bonobo_view_unregister_verb:
 * @view: A BonoboView object.
 * @verb_name: The name of a verb to be unregistered.
 *
 * Unregisters the verb called @verb_name from @view.
 */
void
bonobo_view_unregister_verb (BonoboView *view, const char *verb_name)
{
	gchar *original_key;

	g_return_if_fail (view != NULL);
	g_return_if_fail (BONOBO_IS_VIEW (view));
	g_return_if_fail (verb_name != NULL);

	if (! g_hash_table_lookup_extended (view->priv->verb_callbacks, verb_name,
					    (gpointer *) &original_key, NULL))
		return;
	g_hash_table_remove (view->priv->verb_callbacks, verb_name);
	g_hash_table_remove (view->priv->verb_callback_closures, verb_name);
	g_free (original_key);
}

#ifdef STALE_NOT_USED
/**
 * bonobo_view_execute_verb:
 * @view: A BonoboView object.
 * @verb_name: The name of the verb to execute on @view.
 *
 * Executes the verb specified by @verb_name on @view, emitting a
 * "do_verb" signal and calling the registered verb callback, if one
 * exists for @verb_name on @view.
 */
void
bonobo_view_execute_verb (BonoboView *view, const char *verb_name)
{
	BonoboViewVerbFunc callback;

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
	callback = g_hash_table_lookup (view->priv->verb_callbacks, verb_name);
	if (callback != NULL) {
		void *user_data;

		user_data = g_hash_table_lookup (view->priv->verb_callback_closures, verb_name);

		(*callback) (view, (const char *) verb_name, user_data);
	}
}

static void
bonobo_view_verb_selected_cb (BonoboUIHandler *uih, void *user_data,
			     const char *path)
{
	BonoboView  *view = BONOBO_VIEW (user_data);
	const char *verb_name;

	g_assert (path != NULL);

	/*
	 * Extract the verb name from the selected verb.
	 */
	verb_name = path + 1;

	/*
	 * Execute it.
	 */
	bonobo_view_execute_verb (view, verb_name);

	/*
	 * Store the verb name.
	 */
	gtk_object_set_data (GTK_OBJECT (view), "view_executed_verb_name",
			     g_strdup (verb_name));
	
}
#endif /* STALE_NOT_USED */

/**
 * bonobo_view_popup_verbs:
 * @view: A BonoboView object.
 *
 * Creates a popup menu, filling it with the list of verbs supported
 * by @view.  If a verb is selected, it is executed on @view.  Returns
 * a newly-allocated string containing the name of the selected verb,
 * or %NULL if no verb is selected.
 */
char *
bonobo_view_popup_verbs (BonoboView *view)
{
#ifdef STALE_NOT_USED
	BonoboUIHandler *popup;
	const GList *verbs;
	const GList *l;
	char *verb;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_VIEW (view), NULL);
	g_return_val_if_fail (view->embeddable != NULL, NULL);

	/*
	 * Get a list of the available verbs from our embeddable.
	 */
	verbs = bonobo_embeddable_get_verbs (view->embeddable);

	/*
	 * Build the menu.
	 */
	popup = bonobo_ui_handler_new ();
	bonobo_ui_handler_create_popup_menu (popup);

	for (l = verbs; l != NULL; l = l->next) {
		const GnomeVerb *verb = (GnomeVerb *) l->data;
		char *path;

		path = g_strconcat ("/", verb->name, NULL);
		bonobo_ui_handler_menu_new_item (popup, path,
						verb->label, verb->hint,
						-1,
						BONOBO_UI_HANDLER_PIXMAP_NONE, NULL,
						0, (GdkModifierType) 0,
						bonobo_view_verb_selected_cb,
						view);

		g_free (path);
	}

	/*
	 * Pop up the menu.
	 */
	bonobo_ui_handler_do_popup_menu (popup);

	/*
	 * Destroy it.
	 */
	bonobo_object_unref (BONOBO_OBJECT (popup));

	/*
	 * Grab the name of the executed verb.
	 */
	verb = gtk_object_get_data (GTK_OBJECT (view), "view_executed_verb_name");
	gtk_object_remove_data (GTK_OBJECT (view), "view_executed_verb_name");

	return verb;
#else
	return NULL;
#endif /* STALE_NOT_USED */
}
