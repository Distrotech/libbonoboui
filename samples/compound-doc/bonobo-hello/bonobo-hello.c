/* $Id */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <gnome.h>
#include "config.h"
#if USING_OAF
#include <liboaf/liboaf.h>
#else
#include <libgnorba/gnorba.h>
#endif
#include <bonobo.h>

#include "hello-object.h"

static void
init_bonobo_hello_factory (void)
{
#if USING_OAF
	factory =
	    bonobo_embeddable_factory_new ("OAFIID:bonobo-hello-factory:0.4",
					   hello_object_factory,
					   NULL);
#else
	factory =
	    bonobo_embeddable_factory_new ("bonobo-object-factory:hello",
					   hello_object_factory, NULL);
#endif
}

static void
init_server_factory (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_exception_init (&ev);

#if USING_OAF
	gnome_init_with_popt_table (PACKAGE, VERSION,
				    argc, argv, oaf_popt_options, 0, NULL);
	oaf_init (argc, argv);
#else
	gnome_CORBA_init_with_popt_table (PACKAGE, VERSION,	/* Name of the component */
					  &argc, argv,	/* Command-line arguments */
					  NULL, 0, NULL,	/* popt table to parse arguments */
					  GNORBA_INIT_SERVER_FUNC, &ev	/* GNORBA options */
	    );
#endif

	if (!bonobo_init (CORBA_OBJECT_NIL,
			  CORBA_OBJECT_NIL,
			  CORBA_OBJECT_NIL))
		    g_error (_("I could not initialize Bonobo"));

	CORBA_exception_free (&ev);
}

int
main (int argc, char **argv)
{
	init_server_factory (argc, argv);
	init_bonobo_hello_factory ();

	bonobo_main ();

	return 0;
}
