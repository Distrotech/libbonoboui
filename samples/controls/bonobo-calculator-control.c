/*
 * bonobo-calculator-control.c
 *
 * Author:
 *    Michael Meeks (mmeeks@gnu.org)
 *    Nat Friedman  (nat@nat.org)
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 */

#include <config.h>
#include <gnome.h>
#include <libgnomeui/gnome-calculator.h>
#include <bonobo.h>

#include "bonobo-calculator-control.h"

static void
bonobo_calculator_clear (BonoboUIHandler *uih,
			 gpointer         data,
			 const char      *path)
{
	gnome_calculator_clear (data, TRUE);
}

static void
set_prop (BonoboPropertyBag *bag,
	  const BonoboArg   *arg,
	  guint              arg_id,
	  gpointer           user_data)
{
	gnome_calculator_set (user_data, BONOBO_ARG_GET_DOUBLE (arg));
}

static void
get_prop (BonoboPropertyBag *bag,
	  BonoboArg         *arg,
	  guint              arg_id,
	  gpointer           user_data)
{
	GnomeCalculator *calc = user_data;

	BONOBO_ARG_SET_DOUBLE (arg, calc->result);
}

static BonoboObject *
bonobo_calculator_factory (BonoboGenericFactory *Factory, void *closure)
{
	BonoboControl     *control;
	GtkWidget	  *calc;
	BonoboPropertyBag *pb;

	calc = gnome_calculator_new ();
	gtk_widget_show (calc);

	control = bonobo_control_new (calc);

	pb = bonobo_property_bag_new (get_prop, set_prop, calc);
	bonobo_control_set_property_bag (control, pb);

	bonobo_property_bag_add (pb, "value", 1, BONOBO_ARG_DOUBLE, NULL,
				 "Caluculation result", 0);

	bonobo_control_set_automerge (control, TRUE);

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
  		        "OAFIID:bonobo_calculator_factory:0f55cdac-47fc-4d5b-9111-26c84a244fe2",
			bonobo_calculator_factory, NULL);

	if (bonobo_calc_control_factory == NULL)
		g_error ("I could not register a BonoboCalculator factory.");
}

