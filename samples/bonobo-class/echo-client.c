/*
 * Sample user for the Echo Bonobo component
 *
 * Author:
 *   Miguel de Icaza  (miguel@helixcode.com)
 *
 */
#include <config.h>
#include <libgnorba/gnorba.h>
#include <bonobo.h>
#include "Echo.h"

CORBA_Environment ev;

static void
init_bonobo (int argc, char *argv [])
{
	CORBA_exception_init (&ev);
	
	gnome_CORBA_init_with_popt_table (
		"graph", VERSION,
		&argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);

	if (bonobo_init (NULL, NULL, NULL) == FALSE)
		g_error (_("I could not initialize Bonobo"));

	/*
	 * Enable CORBA/Bonobo to start processing requests
	 */
	bonobo_activate ();
}

int 
main (int argc, char *argv [])
{
	BonoboObjectClient *server;
	Demo_Echo echo_server;

	init_bonobo (argc, argv);

	server = bonobo_object_activate ("GOADID:demo:echo", 0);
	if (!server){
		printf ("Could not create an instance of the GOADID:demo:echo component");
		return 1;
	}

	/*
	 * Get the CORBA Object reference from the BonoboObjectClient
	 */
	echo_server = bonobo_object_corba_objref (BONOBO_OBJECT (server));

	/*
	 * Send a message
	 */
	Demo_Echo_echo (echo_server, "This is the message from the client", &ev);

	/*
	 * Notify we are no longer interested in their services:
	 *
	 * We unref once because of the result from QI
	 * We unref once to get rid of the object altogether.
	 */
	Demo_Echo_unref (echo_server, &ev);
	Demo_Echo_unref (echo_server, &ev);
	
	CORBA_exception_free (&ev);

	return 0;
}
