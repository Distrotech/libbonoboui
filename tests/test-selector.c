/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <libbonoboui.h>

int
main (int argc, char *argv[])
{
	gchar *text;

	bonobo_ui_init ("BonoboSelector Test", VERSION, &argc, argv);

	text = bonobo_selector_select_id (_("Select an object"), NULL);
	g_print ("OAFIID: '%s'\n", text ? text : "<Null>");

	g_free (text);
	
	return 0;
}
