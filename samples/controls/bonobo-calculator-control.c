/*
 * bonobo-calculator-control.c
 *
 * Copyright 1999, Helix Code, Inc.
 *
 * Author:
 *   Michael Meeks (mmeeks@gnu.org)
 */

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo/gnome-bonobo.h>

#include <libgnomeui/gnome-calculator.h>

#include "bonobo-calculator-control.h"

static GnomeObject *
bonobo_calculator_factory (GnomeGenericFactory *Factory, void *closure)
{
	GnomeControl      *control;
	GtkWidget	  *calc;

	/* Create the control. */
	calc = gnome_calculator_new ();
	gtk_widget_show (calc);

	control = gnome_control_new (calc);

	return GNOME_OBJECT (control);
}

void
bonobo_calculator_factory_init (void)
{
	static GnomeGenericFactory *bonobo_calc_control_factory = NULL;

	if (bonobo_calc_control_factory != NULL)
		return;

	bonobo_calc_control_factory =
		gnome_generic_factory_new (
			"control-factory:calculator",
			bonobo_calculator_factory, NULL);

	if (bonobo_calc_control_factory == NULL)
		g_error ("I could not register a BonoboCalculator factory.");
}
