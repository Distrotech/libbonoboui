/*
 * Sample user for the Echo Bonobo component
 *
 * Author:
 *   Miguel de Icaza  (miguel@helixcode.com)
 *
 */


#include <config.h>
#include <gnome.h>
#ifdef USING_OAF
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif
#include <bonobo.h>
#include "Echo.h"

static void
init_bonobo (int argc, char *argv [])
{
	CORBA_ORB orb;
#ifndef USING_OAF
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	
	gnome_CORBA_init_with_popt_table (
		"echo-client", "1.0",
		&argc, argv, NULL, 0, NULL,
		GNORBA_INIT_SERVER_FUNC, &ev);

	orb = gnome_CORBA_ORB ();

	CORBA_exception_free (&ev);
#else
        gnome_init_with_popt_table (
		"echo-client", "1.0",
		argc, argv,
		oaf_popt_options, 0, NULL); 

	orb = oaf_init (argc, argv);
#endif

	if (!bonobo_init (orb, CORBA_OBJECT_NIL,
			  CORBA_OBJECT_NIL))
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
	Demo_Echo           echo_server;
	CORBA_Environment   ev;
	char               *obj_id;

	init_bonobo (argc, argv);

#if USING_OAF
	obj_id = "OAFIID:demo_echo:fe45dab2-ae27-45e9-943d-34a49eefca96";
		
#else
	obj_id = "GOADID:demo_echo";
#endif
	server = bonobo_object_activate (obj_id, 0);

	if (!server) {
		printf ("Could not create an instance of the %s component", obj_id);
		return 1;
	}

	CORBA_exception_init (&ev);

	/*
	 * Get the CORBA Object reference from the BonoboObjectClient
	 */
	echo_server = bonobo_object_corba_objref (BONOBO_OBJECT (server));

	/*
	 * Send a message
	 */
	Demo_Echo_echo (echo_server, "This is the message from the client\n", &ev);

	CORBA_exception_free (&ev);

	bonobo_object_unref (server);
	
	return 0;
}
