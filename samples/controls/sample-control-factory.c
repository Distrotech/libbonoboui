/**
 * sample-control-factory.c
 *
 * Copyright 1999, Helix Code, Inc.
 * 
 * Author:
 *   Nat Friedman (nat@nat.org)
 *
 */

#include <config.h>
#include <gnome.h>

#if USING_OAF
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif

#include <bonobo.h>

#include "bonobo-clock-control.h"
#include "bonobo-calculator-control.h"

CORBA_Environment ev;
CORBA_ORB orb;

static void
init_bonobo (int argc, char **argv)
{

#if USING_OAF
        gnome_init_with_popt_table("sample-control-factory", "0.0",
				   argc, argv,
				   oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);
#else
	gnome_CORBA_init_with_popt_table (
      	"sample-control-factory", "0.0",
	&argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);
	orb = gnome_CORBA_ORB ();
#endif
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo"));
}

int
main (int argc, char **argv)
{
	CORBA_exception_init (&ev);

	init_bonobo (argc, argv);

	bonobo_clock_factory_init ();
	bonobo_calculator_factory_init ();

	bonobo_main ();

	return 0;
}
