/*
 * bonobo-clock-control.c
 *
 * Authors:
 *    Nat Friedman  (nat@helixcode.com)
 *    Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999, 2000, Helix Code, Inc.
 */

#include <config.h>
#include <gnome.h>
#include <bonobo.h>

#include <libgnomeui/gtk-clock.h>

#include "bonobo-clock-control.h"

enum {
	PROP_RUNNING
} MyArgs;

#define RUNNING_KEY  "Clock::Running"

static void
get_prop (BonoboPropertyBag *bag,
	  BonoboArg         *arg,
	  guint              arg_id,
	  gpointer           user_data)
{
	GtkObject *clock = user_data;

	switch (arg_id) {

	case PROP_RUNNING:
	{
		gboolean b = GPOINTER_TO_UINT (gtk_object_get_data (clock, RUNNING_KEY));
		BONOBO_ARG_SET_BOOLEAN (arg, b);
		break;
	}

	default:
		g_warning ("Unhandled arg %d", arg_id);
		break;
	}
}

static void
set_prop (BonoboPropertyBag *bag,
	  const BonoboArg   *arg,
	  guint              arg_id,
	  gpointer           user_data)
{
	GtkClock *clock = user_data;

	switch (arg_id) {

	case PROP_RUNNING:
	{
		guint i;

		i = BONOBO_ARG_GET_BOOLEAN (arg);

		if (i)
			gtk_clock_start (clock);
		else
			gtk_clock_stop (clock);

		gtk_object_set_data (GTK_OBJECT (clock), RUNNING_KEY,
				     GUINT_TO_POINTER (i));
		break;
	}

	default:
		g_warning ("Unhandled arg %d", arg_id);
		break;
	}
}

static BonoboObject *
bonobo_clock_factory (BonoboGenericFactory *Factory, void *closure)
{
	BonoboPropertyBag  *pb;
	BonoboControl      *control;
	GtkWidget	   *clock;

	/* Create the control. */
	clock = gtk_clock_new (GTK_CLOCK_INCREASING);
	gtk_clock_start (GTK_CLOCK (clock));
	gtk_widget_show (clock);
	gtk_object_set_data (GTK_OBJECT (clock), RUNNING_KEY,
			     GUINT_TO_POINTER (1));

	control = bonobo_control_new (clock);

	/* Create the properties. */
	pb = bonobo_property_bag_new (get_prop, set_prop, clock);
	bonobo_control_set_property_bag (control, pb);

	bonobo_property_bag_add (pb, "running", PROP_RUNNING,
				 BONOBO_ARG_BOOLEAN, NULL,
				 "Whether or not the clock is running", 0);

	return BONOBO_OBJECT (control);
}

/*
 * A test widget.
 */
static BonoboObject *
bonobo_entry_factory (BonoboGenericFactory *Factory, void *closure)
{
	BonoboPropertyBag  *pb;
	BonoboControl      *control;
	GtkWidget	   *entry;

	/* Create the control. */
	entry = gtk_entry_new ();
	gtk_widget_show (entry);

	control = bonobo_control_new (entry);
	pb = bonobo_property_bag_new (NULL, NULL, NULL);
	bonobo_control_set_property_bag (control, pb);
	bonobo_property_bag_add_gtk_args (pb, GTK_OBJECT (entry));

	return BONOBO_OBJECT (control);
}

void
bonobo_clock_factory_init (void)
{
	static BonoboGenericFactory *bonobo_clock_control_factory = NULL;
	static BonoboGenericFactory *bonobo_entry_control_factory = NULL;

	if (bonobo_clock_control_factory != NULL)
		return;

	bonobo_clock_control_factory =
		bonobo_generic_factory_new (
			"OAFIID:bonobo_clock_factory:ec4961f3-7a16-4ace-9463-b112e4bc4186",
			bonobo_clock_factory, NULL);

	if (bonobo_clock_control_factory == NULL)
		g_error ("I could not register a BonoboClock factory.");

	bonobo_entry_control_factory =
		bonobo_generic_factory_new (
			"OAFIID:bonobo_entry_factory:ef3e3c33-43e2-4f7c-9ca9-9479104608d6",
			bonobo_entry_factory, NULL);

	if (bonobo_entry_control_factory == NULL)
		g_error ("I could not register an Entry factory.");
}
