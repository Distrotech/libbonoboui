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

#include "hello-object.h"

/* Implemented Bonobo interfaces */
#include "hello-object-io.h"
#include "hello-object-print.h"

#include "hello-view.h"

static int register_interfaces (Hello * obj);

static void hello_object_destroy (Hello * obj);

/* Instance counting */
static int hello_object_num = 0;

static void
hello_object_destroy (Hello * obj)
{
	/* Destroy the instance */
	hello_model_destroy (obj);

	/* Destroy the factory if there are no more instances */
	if ((--hello_object_num) <= 0)
		if (factory) {
			bonobo_object_unref (BONOBO_OBJECT (factory));
			factory = NULL;
			gtk_main_quit ();
		}
}

/* Register CORBA interfaces we implement */
static int
register_interfaces (Hello * obj)
{
	BonoboPersistStream *stream;
	BonoboPrint *print;

	/* Register the Bonobo::PersistStream interface. */
	stream = bonobo_persist_stream_new (hello_object_pstream_load,
					    hello_object_pstream_save,
					    obj);
	g_return_val_if_fail (stream, -1);

	bonobo_object_add_interface (BONOBO_OBJECT (obj->bonobo_object),
				     BONOBO_OBJECT (stream));

	/* Register the Bonobo::Print interface */
	print = bonobo_print_new (hello_object_print, obj);
	g_return_val_if_fail (print, -1);

	bonobo_object_add_interface (BONOBO_OBJECT (obj->bonobo_object),
				     BONOBO_OBJECT (print));


	return 0;
}

/* The "factory" is the function creating new Object instances */
BonoboObject *
hello_object_factory (BonoboEmbeddableFactory * this, void *data)
{
	BonoboEmbeddable *bonobo_object;
	Hello *obj;

	g_return_val_if_fail (obj = g_new0 (Hello, 1), NULL);

	hello_object_num++;

	/* Creates the BonoboObject server */
	bonobo_object = bonobo_embeddable_new (hello_view_factory, obj);
	if (bonobo_object == NULL) {
		g_free (obj);
		return NULL;
	}

	/* Install destructor */
	gtk_signal_connect (GTK_OBJECT (bonobo_object), "destroy",
			    GTK_SIGNAL_FUNC (hello_object_destroy),
			    bonobo_object);

	/* Initialize object data */
	hello_model_init (obj, bonobo_object);

	/* Register CORBA interfaces */
	if (register_interfaces (obj)) {
		gtk_object_unref (GTK_OBJECT (obj->bonobo_object));
		g_free (obj);
		return NULL;
	}

	return BONOBO_OBJECT (bonobo_object);
}
