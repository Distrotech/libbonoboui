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
 */
#include <config.h>
#include <stdio.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/gnome-client-site.h>
#include <bonobo/gnome-embeddable.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdktypes.h>
#include <gtk/gtksocket.h>

enum {
	SHOW_WINDOW,
	QUEUE_RESIZE,
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

	gtk_signal_emit (
		GTK_OBJECT (object),
		gnome_client_site_signals [SHOW_WINDOW],
		shown);
}

static GNOME_Moniker
impl_GNOME_client_site_get_moniker (PortableServer_Servant servant,
				    GNOME_Moniker_type which,
				    CORBA_Environment *ev)
{
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (gnome_object_from_servant (servant));
	GnomeMoniker *container_moniker;

	container_moniker = gnome_container_get_moniker (client_site->container);

	switch (which){
	case GNOME_Moniker_CONTAINER:
		if (container_moniker == NULL)
			return CORBA_OBJECT_NIL;

		return CORBA_Object_duplicate (
			gnome_object_corba_objref (GNOME_OBJECT (container_moniker)), ev);
			
	case GNOME_Moniker_OBJ_RELATIVE:
	case GNOME_Moniker_OBJ_FULL:
		g_warning ("Implement me!\n");
		return CORBA_OBJECT_NIL;
	}
	g_warning ("Unknown GNOME_Moniker_type");
	return CORBA_OBJECT_NIL;
}

static void
impl_GNOME_client_site_queue_resize (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeObject *object = gnome_object_from_servant (servant);

	gtk_signal_emit (GTK_OBJECT (object),
			 gnome_client_site_signals [QUEUE_RESIZE]);
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
impl_GNOME_client_site__destroy (PortableServer_Servant servant, CORBA_Environment *ev)
{
	POA_GNOME_ClientSite__fini ((POA_GNOME_ClientSite *) servant, ev);
	g_free (servant);
}

static PortableServer_ServantBase__epv gnome_client_site_base_epv =
{
	NULL,
	&impl_GNOME_client_site__destroy,
	NULL,
};

POA_GNOME_ClientSite__epv gnome_client_site_epv =
{
	NULL,
	&impl_GNOME_client_site_get_container,
	&impl_GNOME_client_site_show_window,
	&impl_GNOME_client_site_get_moniker,
	&impl_GNOME_client_site_queue_resize,
	&impl_GNOME_client_site_save_object,
};

static POA_GNOME_ClientSite__vepv gnome_client_site_vepv =
{
	&gnome_client_site_base_epv,
	&gnome_object_epv,
	&gnome_client_site_epv,
};

static void
gnome_client_site_destroy (GtkObject *object)
{
	GtkObjectClass *object_class;
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);
	GnomeObject *gnome_object = GNOME_OBJECT (client_site->bound_object);
	
	object_class = (GtkObjectClass *)gnome_client_site_parent_class;

	gnome_container_remove (
		client_site->container,
		GNOME_OBJECT (object));

	/* Destroy the object on the other end */
	g_warning ("FIXME: Should we unref twice?");

	gtk_object_unref (GTK_OBJECT (gnome_object));
	object_class->destroy (object);
}

static void
default_show_window (GnomeClientSite *cs, CORBA_boolean shown)
{
	cs->child_shown = shown ? 1 : 0;
}

static void
default_queue_resize (GnomeClientSite *cs)
{
}

static void
default_save_object (GnomeClientSite *cs, GNOME_Persist_Status *status)
{
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
				GTK_SIGNAL_OFFSET(GnomeClientSiteClass,show_window), 
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT); 
	gnome_client_site_signals [QUEUE_RESIZE] =
		gtk_signal_new ("queue_resize",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET(GnomeClientSiteClass,queue_resize), 
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	gnome_client_site_signals [SHOW_WINDOW] =
		gtk_signal_new ("save_object",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET(GnomeClientSiteClass,save_object), 
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER); 
	gtk_object_class_add_signals (object_class,
				      gnome_client_site_signals,
				      LAST_SIGNAL);
	
	object_class->destroy = gnome_client_site_destroy;
	class->show_window = default_show_window;
	class->queue_resize = default_queue_resize;
	class->save_object = default_save_object;
}

static void
gnome_client_site_init (GnomeClientSite *client_site)
{
}

static CORBA_Object
create_client_site (GnomeObject *object)
{
	POA_GNOME_ClientSite *servant;

	servant = (POA_GNOME_ClientSite *)g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_client_site_vepv;

	POA_GNOME_ClientSite__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_object_activate_servant (object, servant);
}

/**
 * gnome_client_site_construct:
 * @client_site: The GnomeClientSite object to initialize
 * @corba_client_site: The CORBA server that implements the service
 * @container: a GnomeContainer to bind to.
 *
 * This initializes an object of type GnomeClientSite.  See the description
 * for gnome_client_site_new() for more details.
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
 * Returns: %TRUE if @objecdt was successfully bound to @client_site
 * @client_site.
 */
gboolean
gnome_client_site_bind_embeddable (GnomeClientSite *client_site, GnomeObjectClient *object)
{
	CORBA_Object corba_object;
	GnomeObject *gnome_object;
	
	g_return_val_if_fail (client_site != NULL, FALSE);
	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), FALSE);
	g_return_val_if_fail (GNOME_IS_OBJECT_CLIENT (object), FALSE);

	gnome_object = GNOME_OBJECT (object);
	
	corba_object = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (gnome_object), "IDL:GNOME/Embeddable:1.0",
		&gnome_object->ev);

	if (gnome_object->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;
	
	if (corba_object == CORBA_OBJECT_NIL)
		return FALSE;

	GNOME_Embeddable_set_client_site (
		corba_object, 
		gnome_object_corba_objref (GNOME_OBJECT (client_site)),
		&GNOME_OBJECT (client_site)->ev);
	client_site->bound_object = object;
		
	if (gnome_object->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;
	
	return TRUE;
}

static void
set_remote_window (GtkWidget *socket, GNOME_View view)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_View_set_window (view, GDK_WINDOW_XWINDOW (socket->window), &ev);
	CORBA_exception_free (&ev);
}

static void
destroy_view_frame (GnomeViewFrame *view_frame, GnomeObjectClient *server_object)
{
	server_object->view_frames = g_list_remove (server_object->view_frames, view_frame);
}

static void
size_allocate (GtkWidget *widget, GtkAllocation *allocation, GNOME_View view)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	GNOME_View_size_allocate (view, allocation->width, allocation->height, &ev);
	CORBA_exception_free (&ev);
}

/**
 * gnome_client_site_embeddable_new_view:
 * @client_site: the client site that contains a remote Embeddable
 * object.
 *
 * Creates a ViewFrame and asks the remote @server_object (which must
 * support the GNOME::Embeddable interface) to provide a new view of
 * its data.  The remote @server_object will construct a GnomeView
 * object which corresponds to the new GnomeViewFrame returned by this
 * function.
 * 
 * Returns: A GnomeViewFrame object that contains the view frame for
 * the new view of @server_object.  */
GnomeViewFrame *
gnome_client_site_embeddable_new_view (GnomeClientSite *client_site)
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

	gtk_container_add (GTK_CONTAINER (wrapper), socket);
	gtk_widget_show (GTK_WIDGET (wrapper));

	/*
	 * 3. Now, create the view.
	 */
	CORBA_exception_init (&ev);
 	view = GNOME_Embeddable_new_view (
		gnome_object_corba_objref (GNOME_OBJECT (server_object)),
		gnome_object_corba_objref (GNOME_OBJECT (view_frame)),
		&ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		gtk_object_unref (GTK_OBJECT (socket));
		gtk_object_unref (GTK_OBJECT (view_frame));
		CORBA_exception_free (&ev);
		return NULL;
	}

	gnome_view_frame_bind_to_view (view_frame, view);

	/*
	 * 4. Add this new view frame to the list of ViewFrames for
	 * this embedded component.
	 */
	server_object->view_frames = g_list_prepend (server_object->view_frames, view_frame);
	
	/*
	 * 5. Now wait until the socket->window is realized.
	 */
	gtk_signal_connect (GTK_OBJECT (socket), "realize",
			    GTK_SIGNAL_FUNC (set_remote_window), view);

	gtk_signal_connect (GTK_OBJECT (view_frame), "destroy",
			    GTK_SIGNAL_FUNC (destroy_view_frame), server_object);

	gtk_signal_connect (GTK_OBJECT (wrapper), "size_allocate",
			    GTK_SIGNAL_FUNC (size_allocate), view);
	
	CORBA_exception_free (&ev);		
	return view_frame;
}

/**
 * gnome_client_site_embeddable_get_verbs:
 * @server_object: The pointer to the embedeed server object
 *
 * Returns: A GList containing the GnomeVerb structures for the verbs
 * supported by the Embeddable @server_object.
 *
 * The GnomeVerbs can be deallocated with a call to
 * gnome_embeddable_free_verbs().
 */
GList *
gnome_client_site_embeddable_get_verbs (GnomeClientSite *client_site)
{
	GNOME_Embeddable_verb_list *list;
	GNOME_Embeddable object;
	GnomeObjectClient *server_object;
	GnomeObject *gobject;
	GList *l;
	int i;
	
	g_return_val_if_fail (client_site != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), NULL);
	g_return_val_if_fail (client_site->bound_object != NULL, NULL);

	server_object = client_site->bound_object;

	gobject = GNOME_OBJECT (server_object);
	object = (GNOME_Embeddable) gnome_object_corba_objref (gobject);

	list = GNOME_Embeddable_get_verb_list (object, &gobject->ev);

	if (gobject->ev._major != CORBA_NO_EXCEPTION){
		if (list != CORBA_OBJECT_NIL)
			CORBA_free (list);
		
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
	
	CORBA_free (list);

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
