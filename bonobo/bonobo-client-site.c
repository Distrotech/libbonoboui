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

	return GNOME_OBJECT (client_site->container)->object;
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
		
		return GNOME_OBJECT (container_moniker)->object;
			
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

GNOME_Persist_Status
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
	gnome_object_drop_binding_by_servant (servant);
	g_free (servant);
}

PortableServer_ServantBase__epv gnome_client_site_base_epv =
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

POA_GNOME_ClientSite__vepv gnome_client_site_vepv =
{
	&gnome_client_site_base_epv,
	&gnome_object_epv,
	&gnome_client_site_epv,
};

static void
gnome_client_site_destroy (GtkObject *object)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)gnome_client_site_parent_class;

	gnome_container_remove (
		GNOME_CLIENT_SITE (object)->container,
		GNOME_OBJECT (object));
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

	servant = g_new0 (POA_GNOME_ClientSite, 1);
	servant->vepv = &gnome_client_site_vepv;

	POA_GNOME_ClientSite__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_object_activate_servant (object, servant);
}

GnomeClientSite *
gnome_client_site_construct (GnomeClientSite *client_site, GnomeContainer *container)
{
	GNOME_ClientSite corba_client_site;
	
	corba_client_site = create_client_site (GNOME_OBJECT (client_site));
	if (corba_client_site == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (client_site));
		return NULL;
	}

	GNOME_OBJECT (client_site)->object = GNOME_OBJECT (client_site);
	GNOME_CLIENT_SITE (client_site)->container = container;
	gnome_container_add (container, GNOME_OBJECT (client_site));

	return client_site;
}

GnomeClientSite *
gnome_client_site_new (GnomeContainer *container)
{
	GnomeClientSite *client_site;

	g_return_val_if_fail (container != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CONTAINER (container), NULL);
	
	client_site = gtk_type_new (gnome_client_site_get_type ());
	client_site = gnome_client_site_construct (client_site, container);
	
	return client_site;
}

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

gboolean
gnome_client_site_bind_component (GnomeClientSite *client_site, GnomeObject *object)
{
	CORBA_Object corba_object;
	
	g_return_val_if_fail (client_site != NULL, FALSE);
	g_return_val_if_fail (object != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_CLIENT_SITE (client_site), FALSE);
	g_return_val_if_fail (GNOME_IS_OBJECT (object), FALSE);

	corba_object = GNOME_object_query_interface (
		object->object, "IDL:GNOME/Component:1.0",
		&object->ev);

	if (object->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;
	
	if (corba_object == CORBA_OBJECT_NIL)
		return FALSE;

	GNOME_Component_set_client_site (
		corba_object, 
		GNOME_OBJECT (client_site)->object,
		&GNOME_OBJECT (client_site)->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;
	
	return TRUE;
}

