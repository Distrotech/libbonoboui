/*
 * bonobo-clock-control.c
 *
 * Author:
 *    Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc.
 */

#include <config.h>
#include <string.h>
#include <libbonoboui.h>

static void
activate_cb (GtkEditable *editable, BonoboControl *control)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (
		NULL, 0, GTK_MESSAGE_INFO,
		GTK_BUTTONS_OK,
		"This dialog demonstrates transient dialogs");

	bonobo_control_set_transient_for (
		control, GTK_WINDOW (dialog), NULL);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

BonoboObject *
bonobo_entry_control_new (void)
{
	BonoboPropertyBag  *pb;
	BonoboControl      *control;
	GtkWidget	   *entry;

	/* Create the control. */
	entry = gtk_entry_new ();
	gtk_widget_show (entry);

	control = bonobo_control_new (entry);
	pb = bonobo_property_bag_new (NULL, NULL, NULL);
	bonobo_control_set_properties (control, BONOBO_OBJREF (pb), NULL);
	bonobo_object_unref (BONOBO_OBJECT (pb));

	gtk_signal_connect (
		GTK_OBJECT (entry), "activate",
		GTK_SIGNAL_FUNC (activate_cb), control);

	bonobo_property_bag_add_gtk_args (pb, G_OBJECT (entry));

	return BONOBO_OBJECT (control);
}

static BonoboObject *
control_factory (BonoboGenericFactory *this,
		 const char           *object_id,
		 void                 *data)
{
	BonoboObject *object = NULL;
	
	g_return_val_if_fail (object_id != NULL, NULL);

	if (!strcmp (object_id, "OAFIID:Bonobo_Sample_Entry"))

		object = bonobo_entry_control_new ();

	return object;
}

BONOBO_OAF_FACTORY_MULTI ("OAFIID:Bonobo_Sample_ControlFactory",
			  "bonobo-sample-controls", VERSION,
			  control_factory,
			  NULL)