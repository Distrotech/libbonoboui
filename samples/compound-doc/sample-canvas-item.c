/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <gnome.h>
#include <liboaf/liboaf.h>

#include <bonobo/Bonobo.h>
#include <bonobo/bonobo.h>

CORBA_Environment ev;
CORBA_ORB orb;

static BonoboCanvasComponent *
item_factory (BonoboEmbeddable *bonobo_object, GnomeCanvas *canvas, void *data)
{
	GnomeCanvasItem *item;

	item = gnome_canvas_item_new (
		GNOME_CANVAS_GROUP (gnome_canvas_root (canvas)),
		gnome_canvas_rect_get_type (),
		"x1", 0.0,
		"y1", 0.0,
		"x2", 20.0,
		"y2", 20.0,
		"outline_color", "red",
		"fill_color", "blue",
		NULL);

	return bonobo_canvas_component_new (item);
}

static BonoboObject *
bonobo_item_factory (BonoboEmbeddableFactory *factory, void *closure)
{
	BonoboEmbeddable *server;

	printf ("I am in item factory\n");
	server =  bonobo_embeddable_new_canvas_item (item_factory, NULL);
	if (server == NULL)
		g_error ("Can not create bonobo_embeddable");

	return (BonoboObject*) server;
}

static void
init_server_factory (void)
{
	bonobo_embeddable_factory_new (
		"OAFIID:test_canvas_item_factory:db20642e-25a0-46d0-bbe0-84b79ab64e05", bonobo_item_factory, NULL);
}

int
main (int argc, char *argv [])
{
	CORBA_exception_init (&ev);

        gnome_init_with_popt_table("MyServer", "1.0",
				   argc, argv,
				   oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error ("Can not bonobo_init");

	init_server_factory ();
	bonobo_main ();
	CORBA_exception_free (&ev);

	return 0;
}
