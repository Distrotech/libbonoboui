/* $Id$ */
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
#include <liboaf/liboaf.h>
#include <bonobo.h>

#include "hello-embeddable.h"

static BonoboObject*
hello_embeddable_factory (BonoboGenericFactory *f, gpointer data)
{
	HelloBonoboEmbeddable *embeddable;

	embeddable = gtk_type_new (HELLO_BONOBO_EMBEDDABLE_TYPE);

	g_return_val_if_fail(embeddable != NULL, NULL);

	embeddable = hello_bonobo_embeddable_construct (embeddable);

	return BONOBO_OBJECT (embeddable);
}

static void
hello_bonobo_init (void)
{
	BonoboGenericFactory *factory;
	
	factory = bonobo_generic_factory_new (
			"OAFIID:Bonobo_Sample_Hello_EmbeddableFactory",
			hello_embeddable_factory, NULL);
	if (!factory)
		g_warning ("Couldn't register hello object factory");
	else 
		bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (factory));
}

static void
server_factory_init (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_ORB orb;

	CORBA_exception_init (&ev);

	gnome_init_with_popt_table (PACKAGE, VERSION,
				    argc, argv, oaf_popt_options, 0, NULL);
	orb = oaf_init (argc, argv);

	if (!bonobo_init (orb,
			  CORBA_OBJECT_NIL,
			  CORBA_OBJECT_NIL))
		    g_error (_("I could not initialize Bonobo"));

	CORBA_exception_free (&ev);
}

int
main (int argc, char **argv)
{
	server_factory_init (argc, argv);
	hello_bonobo_init ();

	bonobo_main ();

	return 0;
}

