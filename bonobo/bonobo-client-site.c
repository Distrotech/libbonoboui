/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME ClientSite object.
 *
 * A GnomeClientSite object acts as the point-of-contact for an
 * embedded component: the contained GNOME::Embeddable object
 * communicates with the GnomeClientSite when it wants to talk to its
 * container.  There must be a one-to-one mapping between
 * GnomeClientSite objects and embedding GnomeEmbeddable components.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/gnome-client-site.h>
#include <bonobo/gnome-embeddable.h>
#include <bonobo/gnome-bonobo-item.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdktypes.h>
#include <gtk/gtksocket.h>

POA_GNOME_ClientSite__vepv gnome_client_site_vepv;

enum {
	SHOW_WINDOW,
	SAVE_OBJECT,
	LAST_SIGNAL
};

static GnomeObjectClass *gnome_client_site_parent_class;
static guint gnome_client_site_signals [LAST_SIGNAL];

static GNOME_Container
impl_GNOME_client_site_get_container (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeObject *object = gnome_object_from_servant (servant);
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);

	return CORBA_Object_duplicate (gnome_object_corba_objref (
		GNOME_OBJECT (client_site->container)), ev);
}

static void
impl_GNOME_client_site_show_window (PortableServer_Servant servant, CORBA_boolean shown,
				    CORBA_Environment *ev)
{
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (gnome_object_from_servant (servant));
	GnomeObject *object = GNOME_OBJECT (client_site);

	gtk_signal_emit (GTK_OBJECT (object),
			 gnome_client_site_signals [SHOW_WINDOW],
			 shown);
}

static GNOME_Persist_Status
impl_GNOME_client_site_save_object (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeObject *object = gnome_object_from_servant (servant);
	GNOME_Persist_Status status;

	status = GNOME_Persist_SAVE_OK;
	
	gtk_signal_emit (GTK_OBJECT (object),
			 gnome_client_site_signals [SAVE_OBJECT],
			 &status);
	return status;
}

static void
gnome_client_site_destroy (GtkObject *object)
{
	GtkObjectClass *object_class;
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);
	
	object_class = (GtkObjectClass *)gnome_client_site_parent_class;

	/*
	 * Destroy all the view frames.
	 */
	while (client_site->view_frames) {
		GnomeViewFrame *view_frame = GNOME_VIEW_FRAME (client_site->view_frames->data);

		gnome_object_destroy (GNOME_OBJECT (view_frame));
	}

	/*
	 * Destroy all canvas items
	 */
	while (client_site->canvas_items) {
		GnomeBonoboItem *item = GNOME_BONOBO_ITEM (client_site->canvas_items->data);

		gnome_object_destroy (GNOME_OBJECT (item));
	}

	gnome_container_remove (client_site->container, GNOME_OBJECT (object));

	object_class->destroy (object);
}

static void
default_show_window (GnomeClientSite *cs, CORBA_boolean shown)
{
	cs->child_shown = shown ? 1 : 0;
}

static void
default_save_object (GnomeClientSite *cs, GNOME_Persist_Status *status)
{
}

/**
 * gnome_client_site_get_epv:
 *
 */
POA_GNOME_ClientSite__epv *
gnome_client_site_get_epv (void)
{
	POA_GNOME_ClientSite__epv *epv;

	epv = g_new0 (POA_GNOME_ClientSite__epv, 1);

	epv->get_container = impl_GNOME_client_site_get_container;
	epv->show_window   = impl_GNOME_client_site_show_window;
	epv->save_object   = impl_GNOME_client_site_save_object;

	return epv;
}

static void
init_client_site_corba_class ()
{
	gnome_client_site_vepv.GNOME_Unknown_epv = gnome_object_get_epv ();
	gnome_client_site_vepv.GNOME_ClientSite_epv = gnome_client_site_get_epv ();
}

static void
gnome_client_site_class_init (GnomeClientSiteClass *class)
{
	GnomeObjectClass *gobject_class = (GnomeObjectClass *) class;
	GtkObjectClass *object_class = (GtkObjectClass *) gobject_class;
	
	gnome_client_site_parent_class = gtk_type_class (gnome_object_get_type ());

	gnome_client_site_signals [SHOW_WINDOW] =
		gtk_signal_new ("show_window",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeClientSiteClass, show_window), 
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT); 
	gnome_client_site_signals [SAVE_OBJECT] =
		gtk_signal_new ("save_object",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeClientSiteClass, save_object), 
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER); 
	gtk_object_class_add_signals (object_class,
				      gnome_client_site_signals,
				      LAST_SIGNAL);
	
	object_class->destroy = gnome_client_site_destroy;
	class->show_window = default_show_window;
	class->save_object = default_save_object;

	init_client_site_corba_class ();
}

static void
gnome_client_site_init (GnomeClientSite *client_site)
{
}

static CORBA_Object
create_client_site (GnomeObject *object)
{
	POA_GNOME_ClientSite *servant;
	CORBA_Environment ev;

	servant = (POA_GNOME_ClientSite *)g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_client_site_vepv;

	CORBA_exception_init (&ev);

	POA_GNOME_ClientSite__init ( (PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		CORBA_exception_free (&ev);
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);

	return gnome_object_activate_servant (object, servant);

}
/**
 * gnome_client_site_construct:
 * @client_site: The GnomeClientSite object to initialize
 * @corba_client_site: The CORBA server that implements the service
 * @container: a GnomeContainer to bind to.
 *
 * This initializes an object of type GnomeClientSite.  See the description
 * for gnome_client_site_new () for more details.
 *
 * Returns: the constructed GnomeClientSite @client_site.
 */
GnomeClientSite *
gnome_client_site_construct (GnomeClientSite  *client_site,
			     GNOME_ClientSite  corba_client_site,
			     GnomeContainer   *container)
{
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);
	g_return_val_if_fail (container != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTAINER (container), NULL);
	g_return_val_if_fail (corba_client_site != CORBA_OBJECT_NIL, NULL);
	
	gnome_object_construct (GNOME_OBJECT (client_site), corba_client_site);
	
	GNOME_CLIENT_SITE (client_site)->container = container;
	gnome_container_add (container, GNOME_OBJECT (client_site));

	return client_site;
}

/**
 * gnome_client_site_new:
 * @container: The container to which this client_site belongs.
 *
 * Container programs should provide a GnomeClientSite GTK object (ie,
 * a GNOME::ClientSite CORBA server) for each Embeddable which they
 * embed.  This is the contact end point for the remote
 * GNOME::Embeddable object.
 *
 * This routine creates a new GnomeClientSite.
 *
 * Returns: The activated GnomeClientSite object bound to the @container
 * container.
 */
GnomeClientSite *
gnome_client_site_new (GnomeContainer *container)
{
	GNOME_ClientSite corba_client_site;
	GnomeClientSite *client_site;

	g_return_val_if_fail (container != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTAINER (container), NULL);
	
	client_site = gtk_type_new (gnome_client_site_get_type ());
	corba_client_site = create_client_site (GNOME_OBJECT (client_site));
	if (corba_client_site == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (client_site));
		return NULL;
	}

	client_site = gnome_client_site_construct (client_site, corba_client_site, container);
	
	return client_site;
}

/**
 * gnome_client_site_get_type:
 *
 * Returns: The GtkType for the GnomeClient class.
 */
GtkType
gnome_client_site_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/ClientSite:1.0",
			sizeof (GnomeClientSite),
			sizeof (GnomeClientSiteClass),
			 (GtkClassInitFunc) gnome_client_site_class_init,
			 (GtkObjectInitFunc) gnome_client_site_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			 (GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/** 
 * gnome_client_site_bind_embeddable:
 * @client_site: the client site to which the remote Embeddable object will be bound.
 * @object: The remote object which supports the GNOME::Embeddable interface.
 *
 * This routine binds a remote Embeddable object to a local
 * GnomeClientSite object.  The idea is that there is always a
 * one-to-one mapping between GnomeClientSites and GnomeEmbeddables.
 * The Embeddable uses its GnomeClientSite to communicate with the
 * container in which it is embedded.
 *
 * Returns: %TRUE if @object was successfully bound to @client_site
 * @client_site.
 */
gboolean
gnome_client_site_bind_embeddable (GnomeClientSite *client_site, GnomeObjectClient *object)
{
	CORBA_Object corba_object;
	GnomeObject *gnome_object;
	CORBA_Environment ev;
	
	g_return_val_if_fail (client_site != NULL, FALSE);
	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), FALSE);
	g_return_val_if_fail (GNOME_IS_OBJECT_CLIENT (object), FALSE);

	CORBA_exception_init (&ev);

	gnome_object = GNOME_OBJECT (object);

	corba_object = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (gnome_object), "IDL:GNOME/Embeddable:1.0",
		&ev);

	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (
			gnome_object,
			gnome_object_corba_objref (gnome_object),
			&ev);
		CORBA_exception_free (&ev);
		return FALSE;
	}
	
	if (corba_object == CORBA_OBJECT_NIL) {
		CORBA_exception_free (&ev);
		return FALSE;
	}

	GNOME_Embeddable_set_client_site (
		corba_object, 
		gnome_object_corba_objref (GNOME_OBJECT (client_site)),
		&ev);
		
	if (ev._major != CORBA_NO_EXCEPTION){
		CORBA_exception_free (&ev);
		gnome_object_check_env (gnome_object, corba_object, &ev);
		return FALSE;
	}

	client_site->bound_object = object;

	GNOME_Unknown_unref (gnome_object_corba_objref (gnome_object), &ev);
	
	CORBA_exception_free (&ev);

	return TRUE;
}

/**
 * gnome_client_site_get_embeddable:
 * @client_site: A GnomeClientSite object which is bound to a remote
 * GnomeObject server.
 *
 * Returns: The GnomeObjectClient object which corresponds to the
 * remote GnomeObject to which @client_site is bound.
 */
GnomeObjectClient *
gnome_client_site_get_embeddable (GnomeClientSite *client_site)
{
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);

	return client_site->bound_object;
}

static void
set_remote_window (GtkWidget *socket, GnomeViewFrame *view_frame)
{
	GNOME_View view = gnome_view_frame_get_view (view_frame);
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_View_set_window (view, GDK_WINDOW_XWINDOW (socket->window), &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
		gnome_object_check_env (GNOME_OBJECT (view_frame), view, &ev);

	CORBA_exception_free (&ev);
}

static void
destroy_view_frame (GnomeViewFrame *view_frame, GnomeClientSite *client_site)
{
	/*
	 * Remove this view frame.
	 */
	client_site->view_frames = g_list_remove (client_site->view_frames, view_frame);
}

/**
 * gnome_client_site_new_view_full:
 * @client_site: the client site that contains a remote Embeddable object.
 * @visible_cover: %TRUE if the cover should draw a border when it is active.
 * @active_view: %TRUE if the view should be uncovered when it is created.
 *
 * Creates a ViewFrame and asks the remote @server_object (which must
 * support the GNOME::Embeddable interface) to provide a new view of
 * its data.  The remote @server_object will construct a GnomeView
 * object which corresponds to the new GnomeViewFrame returned by this
 * function.
 * 
 * Returns: A GnomeViewFrame object that contains the view frame for
 * the new view of @server_object.
 */
GnomeViewFrame *
gnome_client_site_new_view_full (GnomeClientSite *client_site,
				 gboolean visible_cover,
				 gboolean active_view)
{
	GnomeObjectClient *server_object;
	GnomeViewFrame *view_frame;
	GnomeWrapper *wrapper;
	GtkWidget *socket;
	GNOME_View view;

	CORBA_Environment ev;

	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);
	g_return_val_if_fail (client_site->bound_object != NULL, NULL);

	server_object = client_site->bound_object;

	/*
	 * 1. Get the end points where the containee is embedded.
	 */
	socket = gtk_socket_new ();
	gtk_widget_show (socket);

	/*
	 * 2. Create the view frame.
	 */
	view_frame = gnome_view_frame_new (client_site);
	wrapper = GNOME_WRAPPER (gnome_view_frame_get_wrapper (view_frame));
	gnome_wrapper_set_visibility (wrapper, visible_cover);
	gnome_wrapper_set_covered (wrapper, ! active_view);

	gtk_container_add (GTK_CONTAINER (wrapper), socket);

	/*
	 * 3. Now, create the view.
	 */
	CORBA_exception_init (&ev);
 	view = GNOME_Embeddable_new_view (
		gnome_object_corba_objref (GNOME_OBJECT (server_object)),
		gnome_object_corba_objref (GNOME_OBJECT (view_frame)),
		&ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_object_check_env (
			GNOME_OBJECT (client_site),
			gnome_object_corba_objref (GNOME_OBJECT (server_object)),
			&ev);
		gtk_object_unref   (GTK_OBJECT (socket));
		gnome_object_unref (GNOME_OBJECT (view_frame));
		CORBA_exception_free (&ev);
		return NULL;
	}

	gnome_view_frame_bind_to_view (view_frame, view);
	CORBA_Object_release (view, &ev);
	
	/*
	 * 4. Add this new view frame to the list of ViewFrames for
	 * this embedded component.
	 */
	client_site->view_frames = g_list_prepend (client_site->view_frames, view_frame);
	
	/*
	 * 5. Now wait until the socket->window is realized.
	 */
	gtk_signal_connect (GTK_OBJECT (socket), "realize",
			    GTK_SIGNAL_FUNC (set_remote_window), view_frame);

	gtk_signal_connect (GTK_OBJECT (view_frame), "destroy",
			    GTK_SIGNAL_FUNC (destroy_view_frame), client_site);

	CORBA_exception_free (&ev);		
	return view_frame;
}

/**
 * gnome_client_site_new_view:
 * @client_site: the client site that contains a remote Embeddable
 * object.
 *
 * The same as gnome_client_site_new_view_full() with an inactive,
 * visible cover.
 * 
 * Returns: A GnomeViewFrame object that contains the view frame for
 * the new view of @server_object.
 */
GnomeViewFrame *
gnome_client_site_new_view (GnomeClientSite *client_site)
{

	return gnome_client_site_new_view_full (client_site, TRUE, FALSE);
}

static void
canvas_item_destroyed (GnomeCanvasItem *item, GnomeClientSite *client_site)
{
	client_site->canvas_items = g_list_remove (client_site->canvas_items, item);
}
		      
/**
 * gnome_client_site_new_item:
 * @client_site: The client site that contains a remote Embeddable object
 * @group: The Canvas group that will be the parent for the new item.
 *
 */
GnomeCanvasItem *
gnome_client_site_new_item (GnomeClientSite *client_site, GnomeCanvasGroup *group)
{
	GnomeObjectClient *server_object;
	GnomeCanvasItem *item;
		
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);
	g_return_val_if_fail (client_site->bound_object != NULL, NULL);
	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);

	server_object = client_site->bound_object;

	item = gnome_bonobo_item_new (group, server_object);

	/*
	 * 5. Add this new view frame to the list of ViewFrames for
	 * this embedded component.
	 */
	client_site->canvas_items = g_list_prepend (client_site->canvas_items, item);

	gtk_signal_connect (GTK_OBJECT (item), "destroy",
			    GTK_SIGNAL_FUNC (canvas_item_destroyed), client_site);
	
	return item;
}

/**
 * gnome_client_site_get_verbs:
 * @server_object: The pointer to the embedeed server object
 *
 * Returns: A GList containing the GnomeVerb structures for the verbs
 * supported by the Embeddable @server_object.
 *
 * The GnomeVerbs can be deallocated with a call to
 * gnome_embeddable_free_verbs ().
 */
GList *
gnome_client_site_get_verbs (GnomeClientSite *client_site)
{
	GNOME_Embeddable_verb_list *list;
	GNOME_Embeddable object;
	GnomeObjectClient *server_object;
	GnomeObject *gobject;
	CORBA_Environment ev;
	GList *l;
	int i;
	
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);
	g_return_val_if_fail (client_site->bound_object != NULL, NULL);

	CORBA_exception_init (&ev);

	server_object = client_site->bound_object;

	gobject = GNOME_OBJECT (server_object);
	object = (GNOME_Embeddable) gnome_object_corba_objref (gobject);

	list = GNOME_Embeddable_get_verb_list (object, &ev);

	if (ev._major != CORBA_NO_EXCEPTION){
		gnome_object_check_env (GNOME_OBJECT (client_site), object, &ev);
		CORBA_exception_free (&ev);
		return NULL;
	}

	for (l = NULL, i = 0; i < list->_length; i++) {
		GnomeVerb *verb;

		verb = g_new0 (GnomeVerb, 1);
		verb->name = g_strdup (list->_buffer [i].name);
		verb->label = g_strdup (list->_buffer [i].label);
		verb->hint = g_strdup (list->_buffer [i].hint);

		l = g_list_prepend (l, verb);
	}

	CORBA_exception_free (&ev);
	CORBA_free (list);	/* FIXME: This does not make any sense at all to me */

	return l;
}

/**
 * gnome_client_site_free_verbs:
 */
void
gnome_client_site_free_verbs (GList *verbs)
{
	GList *curr;

	if (verbs == NULL)
		return;

	for (curr = verbs; curr != NULL; curr = curr->next) {
		GnomeVerb *verb = (GnomeVerb *) curr->data;

		g_free (verb->name);
		g_free (verb->label);
		g_free (verb->hint);
		g_free (verb);
	}
	
	g_list_free (verbs);
}
