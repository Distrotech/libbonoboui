#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-container.h>
#include <bonobo/gnome-client-site.h>

CORBA_Environment ev;
CORBA_ORB orb;

char *server_repoid = "IDL:Sample/server:1.0";

GnomeContainer *container;

static void
container_add (GnomeContainer *container, char *repoid)
{
	GnomeClientSite *client_site;
	GnomeObject     *object_server;
	
	client_site = gnome_client_site_new (container);
	gnome_container_add (container, GNOME_OBJECT (client_site));

	object_server = gnome_object_activate_with_repo_id (NULL, repoid, 0, NULL);
	if (!object_server){
		g_warning (_("Can not activate object_server\n"));
		return;
	}

	if (!gnome_client_site_bind_component (client_site, object_server)){
		g_warning (_("Can not bind object server to client_site\n"));
		return;
	}
}

void
main (int argc, char *argv [])
{
	CORBA_exception_init (&ev);

	gnome_CORBA_init ("MyShell", "1.0", &argc, argv, 0, &ev);
	orb = gnome_CORBA_ORB ();
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init\n"));

	container = GNOME_CONTAINER (gnome_container_new ());

	container_add (container, server_repoid);
	
	gtk_main ();

	CORBA_exception_free (&ev);
}
