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
#include <bonobo.h>

#include <libgnomeui/gnome-calculator.h>

#include "bonobo-calculator-control.h"

static BonoboObject *
bonobo_calculator_factory (BonoboGenericFactory *Factory, void *closure)
{
	BonoboControl      *control;
	GtkWidget	  *calc;

	/* Create the control. */
	calc = gnome_calculator_new ();
	gtk_widget_show (calc);

	control = bonobo_control_new (calc);

	return BONOBO_OBJECT (control);
}

void
bonobo_calculator_factory_init (void)
{
	static BonoboGenericFactory *bonobo_calc_control_factory = NULL;

	if (bonobo_calc_control_factory != NULL)
		return;

	bonobo_calc_control_factory =
		bonobo_generic_factory_new (
			"control-factory:calculator",
			bonobo_calculator_factory, NULL);

	if (bonobo_calc_control_factory == NULL)
		g_error ("I could not register a BonoboCalculator factory.");
}
