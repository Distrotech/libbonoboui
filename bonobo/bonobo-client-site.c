/**
 * GNOME ClientSite object.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include "bonobo.h"
#include "gnome-container.h"
#include "gnome-clientsite.h"

enum {
	SHOW_WINDOW,
	QUEUE_RESIZE,
	SAVE_OBJECT,
	LAST_SIGNAL
};

static GnomeObjectClass *gnome_client_site_parent_class;

static GNOME_Container
impl_GNOME_client_site_get_container (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeObject *object = gnome_object_from_servant (servant);
	GnomeClientSite *client_site = GNOME_CLIENT_SITE (object);

	return client_site->container;
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
		
		return GNOME_OBJECT (moniker)->object;
			
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
	gnome_object_parent_class->destroy (object);
}

static void
default_show_window (GnomeClientSite *cs, CORBA_boolean shown)
{
	cs->child_shown = shown ? 1 : 0;
}

static void
default_queue_resize (void)
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
