/**
 * GNOME ClientSite object.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <stdio.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/gnome-client-site.h>

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
	GnomeObject *object = gnome_object_from_servant (servant);
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);

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
	GnomeObject *object = gnome_object_from_servant (servant);
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);
	GnomeMoniker *container_moniker;

	container_moniker = gnome_container_get_moniker (client_site->container);

	switch (which){
	case GNOME_Moniker_CONTAINER:
		if (container_moniker == NULL)
			return CORBA_OBJECT_NIL;

		return CORBA_Object_duplicate (
			gnome_object_corba_objref (container_moniker), &ev);
			
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

	gtk_object_unref (GTK_OBJECT (client_site->bound_object));
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
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GnomeObjectClass *gobject_class = (GnomeObjectClass *) class;
	
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

