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

static void
bonobo_calculator_clear (BonoboUIHandler *uih,
			 gpointer         data,
			 const char      *path)
{
	GnomeCalculator *calc = data;

	gnome_calculator_clear (calc, TRUE);
}

static GnomeUIInfo calc_menu[] = {
	GNOMEUIINFO_MENU_NEW_ITEM (N_("_Clear"),
				   N_("Clear the calculator and reset it"),
				   bonobo_calculator_clear, NULL),
	GNOMEUIINFO_END
};

 static GnomeUIInfo control_menus[] = {
	GNOMEUIINFO_SUBTREE (N_("Calculator"), calc_menu),
	GNOMEUIINFO_END
};

static BonoboObject *
bonobo_calculator_factory (BonoboGenericFactory *Factory, void *closure)
{
	BonoboControl   *control;
	GtkWidget	*calc;

 	/* Create the control. */
	calc = gnome_calculator_new ();
	gtk_widget_show (calc);

	control = bonobo_control_new (calc);

	bonobo_control_set_automerge (control, TRUE);
	bonobo_control_set_menus_with_data (control, control_menus, calc);

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
