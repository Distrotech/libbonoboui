/*
 * bonobo-clock-control.c
 *
 * Copyright 1999, Helix Code, Inc.
 *
 * Author:
 *   Nat Friedman (nat@nat.org)
 */

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo/gnome-bonobo.h>

#include <libgnomeui/gtk-clock.h>

#include "bonobo-clock-control.h"

static void
bonobo_clock_control_prop_value_changed_cb (GnomePropertyBag *pb, char *name, char *type,
					    gpointer old_value, gpointer new_value,
					    gpointer user_data)
{
	GtkClock *clock = user_data;

	if (! strcmp (name, "running")) {
		gboolean *b = new_value;

		if (*b)
			gtk_clock_start (clock);
		else
			gtk_clock_stop (clock);
	}
}

static GnomeObject *
bonobo_clock_factory (GnomeGenericFactory *Factory, void *closure)
{
	GnomePropertyBag  *pb;
	GnomeControl      *control;
	GtkWidget	  *clock;
	CORBA_boolean	  *running;

	/* Create the control. */
	clock = gtk_clock_new (GTK_CLOCK_INCREASING);
	gtk_clock_start (GTK_CLOCK (clock));
	gtk_widget_show (clock);

	control = gnome_control_new (clock);

	/* Create the properties. */
	pb = gnome_property_bag_new ();
	gnome_control_set_property_bag (control, pb);

	gtk_signal_connect (GTK_OBJECT (pb), "value_changed",
			    bonobo_clock_control_prop_value_changed_cb,
			    clock);

	running = g_new0 (gboolean, 1);
	*running = TRUE;
	gnome_property_bag_add (pb, "running", "boolean",
				(gpointer) running,
				NULL, "Whether or not the clock is running", 0);

	return GNOME_OBJECT (control);
}

void
bonobo_clock_factory_init (void)
{
	static GnomeGenericFactory *bonobo_clock_control_factory = NULL;

	if (bonobo_clock_control_factory != NULL)
		return;

	bonobo_clock_control_factory =
		gnome_generic_factory_new (
			"control-factory:clock",
			bonobo_clock_factory, NULL);

	if (bonobo_clock_control_factory == NULL) {
		g_error ("I could not register a BonoboClock factory.");
	}
}
