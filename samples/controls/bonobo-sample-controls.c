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

#include <libgnomecanvas/gnome-canvas-widget.h>
#define USE_SCROLLED

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
	GParamSpec        **pspecs;
	guint               n_pspecs;

	/* Create the control. */
	entry = gtk_entry_new ();
	gtk_widget_show (entry);

#ifdef USE_SCROLLED
	{
		GtkWidget *canvas, *scrolled;
		GnomeCanvasItem *item;
		
		gtk_widget_push_visual (gdk_rgb_get_visual ());
		gtk_widget_push_colormap (gdk_rgb_get_cmap ());

		canvas = gnome_canvas_new ();
		gtk_widget_show (canvas);
		
		item = gnome_canvas_item_new (
			gnome_canvas_root (GNOME_CANVAS (canvas)),
			GNOME_TYPE_CANVAS_WIDGET,
			"x", 0.0, "y", 0.0, "width", 100.0,
			"height", 100.0, "widget", entry, NULL);
		gnome_canvas_item_show (item);

		gtk_widget_pop_visual ();
		gtk_widget_pop_colormap ();

		scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);

		gtk_container_add (
			GTK_CONTAINER (scrolled), canvas);
		gtk_widget_show (scrolled);

		control = bonobo_control_new (scrolled);
	}
#else
	control = bonobo_control_new (entry);
#endif
	pb = bonobo_property_bag_new (NULL, NULL, NULL);
	bonobo_control_set_properties (control, BONOBO_OBJREF (pb), NULL);
	bonobo_object_unref (BONOBO_OBJECT (pb));

	gtk_signal_connect (
		GTK_OBJECT (entry), "activate",
		GTK_SIGNAL_FUNC (activate_cb), control);

	pspecs = g_object_class_list_properties (
		G_OBJECT_GET_CLASS (entry), &n_pspecs);

	bonobo_property_bag_map_params (
		pb, G_OBJECT (entry), (const GParamSpec **)pspecs, n_pspecs);

	g_free (pspecs);

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

BONOBO_ACTIVATION_FACTORY ("OAFIID:Bonobo_Sample_ControlFactory",
			   "bonobo-sample-controls-2", VERSION,
			   control_factory,
			   NULL)
