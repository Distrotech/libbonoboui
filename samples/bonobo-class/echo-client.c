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

CORBA_Environment ev;

static void
init_bonobo (int argc, char *argv [])
{
	CORBA_exception_init (&ev);
	
#if USING_OAF
        gnome_init_with_popt_table("echo-client", "1.0",
				   argc, argv,
				   oaf_popt_options, 0, NULL); 

	oaf_init (argc, argv);
#else
	gnome_CORBA_init_with_popt_table (
      	"echo-client", "1.0",
	&argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);
#endif

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

#if USING_OAF
	server = bonobo_object_activate ("OAFIID:demo_echo:fe45dab2-ae27-45e9-943d-34a49eefca96", 0);
#else
	server = bonobo_object_activate ("GOADID:demo_echo", 0);

#endif
	if (!server){
#ifdef USING_OAF
		printf ("Could not create an instance of the OAFIID:demo_echo:fe45dab2-ae27-45e9-943d-34a49eefca96 component");
#else
		printf ("Could not create an instance of the GOADID:demo:echo");
#endif
		return 1;
	}

	/*
	 * Get the CORBA Object reference from the BonoboObjectClient
	 */
	echo_server = bonobo_object_corba_objref (BONOBO_OBJECT (server));

	/*
	 * Send a message
	 */
	Demo_Echo_echo (echo_server, "This is the message from the client\n", &ev);

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
