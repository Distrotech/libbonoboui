/* $Id$ */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "component.h"

#include "component-io.h"
#include "container-filesel.h"

static void activate_request_cb (BonoboViewFrame * view_frame,
				 Component * component);
static void view_activated_cb (BonoboViewFrame * view_frame,
			       gboolean activated, Component * component);
static void deactivate (Component * component);

/* Context menu ("right-click menu") stuff */
static void component_user_context_cb (BonoboViewFrame * view_frame,
				       Component * component);

/* Button callbacks */
static void add_view_cb (GtkWidget * caller, Component * component);
static void del_view_cb (GtkWidget * caller, Component * component);
static void del_cb (GtkWidget * caller, Component * component);
static void fill_cb (GtkWidget * caller, Component * component);

static void
deactivate (Component * component)
{
	SampleApp *container = component->container;

	if (container->curr_view) {
		/*
		 * This just sends a notice to the embedded View that
		 * it is being deactivated.  We will also forcibly
		 * cover it so that it does not receive any Gtk
		 * events.
		 */
		bonobo_view_frame_view_deactivate (container->curr_view);

		/*
		 * Here we manually cover it if it hasn't acquiesced.
		 * If it has consented to be deactivated, then it will
		 * already have notified us that it is inactive, and
		 * we will have covered it and set active_view_frame
		 * to NULL.  Which is why this check is here.
		 */
		if (container->curr_view)
			bonobo_view_frame_set_covered (container->
						       curr_view, TRUE);

		container->curr_view = NULL;
	}
}

void
component_add_view (Component * component)
{
	BonoboViewFrame *view_frame;
	GtkWidget *view_widget;

	/*
	 * Create the remote view and the local ViewFrame.  This also
	 * sets the BonoboUIHandler for this ViewFrame.  That way, the
	 * embedded component can get access to our UIHandler server
	 * so that it can merge menu and toolbar items when it gets
	 * activated.
	 */
	view_frame = bonobo_client_site_new_view (component->client_site,
						  bonobo_object_corba_objref
						  (BONOBO_OBJECT
						   (component->container->
						    ui_handler)));

	/*
	 * Embed the view frame into the application.
	 */
	view_widget = bonobo_view_frame_get_wrapper (view_frame);
	component->views = g_list_append (component->views, view_frame);
	gtk_box_pack_start (GTK_BOX (component->views_hbox), view_widget,
			    FALSE, FALSE, 5);

	/*
	 * The "user_activate" signal will be emitted when the user
	 * double clicks on the "cover".  The cover is a transparent
	 * window which sits on top of the component and keeps any
	 * events (mouse, keyboard) from reaching it.  When the user
	 * double clicks on the cover, the container (that's us)
	 * can choose to activate the component.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "user_activate",
			    GTK_SIGNAL_FUNC (activate_request_cb),
			    component);

	/*
	 * In-place activation of a component is a two-step process.
	 * After the user double clicks on the component, our signal
	 * callback (component_user_activate_request_cb()) asks the
	 * component to activate itself (see
	 * bonobo_view_frame_view_activate()).  The component can then
	 * choose to either accept or refuse activation.  When an
	 * embedded component notifies us of its decision to change
	 * its activation state, the "activated" signal is
	 * emitted from the view frame.  It is at that point that we
	 * actually remove the cover so that events can get through.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "activated",
			    GTK_SIGNAL_FUNC (view_activated_cb),
			    component);

	/*
	 * The "user_context" signal is emitted when the user right
	 * clicks on the wrapper.  We use it to pop up a verb menu.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "user_context",
			    GTK_SIGNAL_FUNC (component_user_context_cb),
			    component);

	/*
	 * Show the component.
	 */
	gtk_widget_show_all (view_widget);

}

void
component_del_view (Component * component)
{
	BonoboViewFrame *last_view;

	if (!component->views)
		return;

	last_view = g_list_last (component->views)->data;
	component->views = g_list_remove (component->views, last_view);

	/* If this is the activated view, deactivate it */
	if (component->container->curr_view == last_view)
		deactivate (component);


	gtk_container_remove (GTK_CONTAINER (component->views_hbox),
			      bonobo_view_frame_get_wrapper (last_view));
	bonobo_object_unref (BONOBO_OBJECT (last_view));
}

void
component_del (Component * component)
{
	bonobo_object_unref (BONOBO_OBJECT (component->server));

	/* Remove from container */
	sample_app_remove_component (component->container, component);
}

void
component_print (Component * component,
		 GnomePrintContext * ctx,
		 gdouble x, gdouble y, gdouble width, gdouble height)
{
	BonoboObjectClient *client = component->server;
	BonoboPrintClient *print_client = bonobo_print_client_get (client);
	BonoboPrintData *print_data;

	if (!print_client)
		return;

	print_data = bonobo_print_data_new (width, height);
	bonobo_print_client_render (print_client, print_data);
	bonobo_print_data_render (ctx, x, y, print_data, 0.0, 0.0);
	bonobo_print_data_free (print_data);
}

static void
activate_request_cb (BonoboViewFrame * view_frame, Component * component)
{
	/*
	 * If there is already an active View, deactivate it.
	 */
	deactivate (component);

	/*
	 * Activate the View which the user clicked on.  This just
	 * sends a request to the embedded View to activate itself.
	 * When it agrees to be activated, it will notify its
	 * ViewFrame, and our view_activated_cb callback will be
	 * called.
	 *
	 * We do not uncover the View here, because it may not wish to
	 * be activated, and so we wait until it notifies us that it
	 * has been activated to uncover it.
	 */
	bonobo_view_frame_view_activate (view_frame);
}

static void
view_activated_cb (BonoboViewFrame * view_frame, gboolean activated,
		   Component * component)
{
	SampleApp *container = component->container;

	if (activated) {
		/*
		 * If the View is requesting to be activated, then we
		 * check whether or not there is already an active
		 * View.
		 */
		if (container->curr_view) {
			g_warning
			    ("View requested to be activated but there is already "
			     "an active View!\n");
			return;
		}

		/*
		 * Otherwise, uncover it so that it can receive
		 * events, and set it as the active View.
		 */
		bonobo_view_frame_set_covered (view_frame, FALSE);
		container->curr_view = view_frame;
	}
	else {
		/*
		 * If the View is asking to be deactivated, always
		 * oblige.  We may have already deactivated it (see
		 * user_activation_request_cb), but there's no harm in
		 * doing it again.  There is always the possibility
		 * that a View will ask to be deactivated when we have
		 * not told it to deactivate itself, and that is
		 * why we cover the view here.
		 */
		bonobo_view_frame_set_covered (view_frame, TRUE);

		if (view_frame == container->curr_view)
			container->curr_view = NULL;
	}
}

static void
component_user_context_cb (BonoboViewFrame * view_frame,
			   Component * component)
{
	char *executed_verb;
	GList *l;

	/*
	 * See if the remote BonoboEmbeddable supports any verbs at
	 * all.
	 */
	l = bonobo_client_site_get_verbs (component->client_site);
	if (l == NULL)
		return;
	bonobo_client_site_free_verbs (l);

	/*
	 * Popup the verb popup and execute the chosen verb.  This
	 * function saves us the work of creating the menu, connecting
	 * the callback, and executing the verb on the remove
	 * BonoboView.  We could implement all this functionality
	 * ourselves if we wanted.
	 */
	executed_verb = bonobo_view_frame_popup_verbs (view_frame);

	g_free (executed_verb);
}

GtkWidget *
component_create_frame (Component * component, gchar * goad_id)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;
	GtkWidget *new_view_button, *del_view_button,
	    *del_comp_button, *fill_comp_button;

	/* Display widgets */
	frame = component->widget = gtk_frame_new (goad_id);
	vbox = gtk_vbox_new (FALSE, 10);
	hbox = gtk_hbox_new (TRUE, 5);
	new_view_button = gtk_button_new_with_label ("New view");
	del_view_button = gtk_button_new_with_label ("Remove view");
	del_comp_button = gtk_button_new_with_label ("Remove component");

	/* The views of the component */
	component->views_hbox = gtk_hbox_new (FALSE, 2);
	gtk_signal_connect (GTK_OBJECT (new_view_button), "clicked",
			    GTK_SIGNAL_FUNC (add_view_cb), component);
	gtk_signal_connect (GTK_OBJECT (del_view_button), "clicked",
			    GTK_SIGNAL_FUNC (del_view_cb), component);
	gtk_signal_connect (GTK_OBJECT (del_comp_button), "clicked",
			    GTK_SIGNAL_FUNC (del_cb), component);

	gtk_container_add (GTK_CONTAINER (hbox), new_view_button);
	gtk_container_add (GTK_CONTAINER (hbox), del_view_button);
	gtk_container_add (GTK_CONTAINER (hbox), del_comp_button);

	if (bonobo_object_client_has_interface (component->server,
						"IDL:Bonobo/PersistStream:1.0",
						NULL)) {
		fill_comp_button =
		    gtk_button_new_with_label ("Fill with stream");
		gtk_container_add (GTK_CONTAINER (hbox), fill_comp_button);

		gtk_signal_connect (GTK_OBJECT (fill_comp_button),
				    "clicked", GTK_SIGNAL_FUNC (fill_cb),
				    component);
	}


	gtk_container_add (GTK_CONTAINER (vbox), component->views_hbox);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	component->goad_id = g_strdup (goad_id);

	return frame;
}


static void
add_view_cb (GtkWidget * caller, Component * component)
{
	component_add_view (component);
}

static void
del_view_cb (GtkWidget * caller, Component * component)
{
	component_del_view (component);
}

static void
del_cb (GtkWidget * caller, Component * component)
{
	component_del (component);
}

static void
load_stream_cb (GtkWidget * caller, Component * component)
{
	GtkWidget *fs = component->container->fileselection;
	gchar *filename = g_strdup (gtk_file_selection_get_filename
				    (GTK_FILE_SELECTION (fs)));
	gtk_widget_destroy (fs);

	if (filename) {
		CORBA_Environment ev;
		Bonobo_PersistStream persist;
		BonoboStream *stream;

		stream =
		    bonobo_stream_fs_open (filename, Bonobo_Storage_READ);

		if (stream == NULL) {
			gchar *error_msg;

			error_msg =
			    g_strdup_printf (_("Could not open file %s"),
					     filename);
			gnome_warning_dialog (error_msg);
			g_free (error_msg);

			return;
		}

		/*
		 * Now get the PersistStream interface off the embedded
		 * component.
		 */
		persist =
		    bonobo_object_client_query_interface (component->
							  server,
							  "IDL:Bonobo/PersistStream:1.0",
							  NULL);

		/*
		 * If the component doesn't support PersistStream (and it
		 * really ought to -- we query it to see if it supports
		 * PersistStream before we even give the user the option of
		 * loading data into it with PersistStream), then we destroy
		 * the stream we created and bail.
		 */
		if (persist == CORBA_OBJECT_NIL) {
			gnome_warning_dialog (_
					      ("The component now claims that it "
					       "doesn't support PersistStream!"));
			bonobo_object_unref (BONOBO_OBJECT (stream));
			g_free (filename);
			return;
		}

		CORBA_exception_init (&ev);

		/*
		 * Load the file into the component using PersistStream.
		 */
		Bonobo_PersistStream_load (persist,
					   (Bonobo_Stream)
					   bonobo_object_corba_objref
					   (BONOBO_OBJECT (stream)),
					   "", &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			gnome_warning_dialog (_
					      ("An exception occured while trying "
					       "to load data into the component with "
					       "PersistStream"));
		}

		/*
		 * Now we destroy the PersistStream object.
		 */
		Bonobo_Unknown_unref (persist, &ev);
		CORBA_Object_release (persist, &ev);

		bonobo_object_unref (BONOBO_OBJECT (stream));

		CORBA_exception_free (&ev);
	}

	g_free (filename);
}

static void
fill_cb (GtkWidget * caller, Component * component)
{
	container_request_file (component->container, FALSE,
				load_stream_cb, component);
}
