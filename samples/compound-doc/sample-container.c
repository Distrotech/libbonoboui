/*
 * sample-container.c
 *
 * This program is a sample Bonobo container. It allows the user to select
 * an arbitrary component for inclusion, and to render an arbritary number of
 * views of that component. It also affords easy testing of PersistStream and
 * PersistFile interfaces.
 *
 * Author:
 *   Nat Friedman (nat@nat.org)
 */

#include <config.h>
#include <gnome.h>
#include <bonobo.h>
#include <libgnorba/gnorba.h>

#include <libgnomeprint/gnome-printer.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-printer-dialog.h>

#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include <libgnomeprint/gnome-print-dialog.h>

#include <bonobo/bonobo-print-client.h>

typedef struct {
	BonoboContainer  *container;
	BonoboUIHandler  *uih;

	BonoboViewFrame  *active_view_frame;

	GtkWidget	*app;
	GtkWidget	*vbox;
} Container;

typedef struct {
	Container	  *container;

	BonoboClientSite   *client_site;
	BonoboObjectClient *server;

	GtkWidget	  *frame;
	GtkWidget	  *views_hbox;
	GtkWidget	  *fs;
} Component;

/*
 * Static prototypes.
 */
static void container_add_embeddable_cmd (GtkWidget *widget, Container *container);
static void container_print_preview_cmd  (GtkWidget *widget, Container *container);
static void container_exit_cmd (GtkWidget *widget, Container *container); 

/*
 * The menus.
 */
static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("_Add a new Embeddable component"), NULL,
		container_add_embeddable_cmd),
	GNOMEUIINFO_ITEM_NONE (
		N_("Print Pre_view"), NULL,
		container_print_preview_cmd),
	GNOMEUIINFO_MENU_EXIT_ITEM (container_exit_cmd, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_main_menu [] = {
	GNOMEUIINFO_MENU_FILE_TREE (container_file_menu),
	GNOMEUIINFO_END
};

/*
 * Invoked when the user selects the "exit" menu item.
 */
static void
container_exit_cmd (GtkWidget *widget, Container *container)
{
	/*
	 * Destroying the container will destroy all the components.
	 */
	bonobo_object_destroy (BONOBO_OBJECT (container->container));

	gtk_main_quit ();
}

static void
component_user_activate_request_cb (BonoboViewFrame *view_frame, Component *component)
{
	Container *container = component->container;

	/*
	 * If there is already an active View, deactivate it.
	 */
        if (container->active_view_frame != NULL) {
		/*
		 * This just sends a notice to the embedded View that
		 * it is being deactivated.  We will also forcibly
		 * cover it so that it does not receive any Gtk
		 * events.
		 */
                bonobo_view_frame_view_deactivate (container->active_view_frame);

		/*
		 * Here we manually cover it if it hasn't acquiesced.
		 * If it has consented to be deactivated, then it will
		 * already have notified us that it is inactive, and
		 * we will have covered it and set active_view_frame
		 * to NULL.  Which is why this check is here.
		 */
		if (container->active_view_frame != NULL)
			bonobo_view_frame_set_covered (container->active_view_frame, TRUE);
									     
		container->active_view_frame = NULL;
	}

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
component_view_activated_cb (BonoboViewFrame *view_frame, gboolean activated, Component *component)
{
	Container *container = component->container;

        if (activated) {
		/*
		 * If the View is requesting to be activated, then we
		 * check whether or not there is already an active
		 * View.
		 */
		if (container->active_view_frame != NULL) {
			g_warning ("View requested to be activated but there is already "
				   "an active View!\n");
			return;
		}

		/*
		 * Otherwise, uncover it so that it can receive
		 * events, and set it as the active View.
		 */
		bonobo_view_frame_set_covered (view_frame, FALSE);
                container->active_view_frame = view_frame;
        } else {
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

		if (view_frame == container->active_view_frame)
			container->active_view_frame = NULL;
        }                                                                       
}

static void
component_user_context_cb (BonoboViewFrame *view_frame, Component *component)
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

static void
component_shutdown (Component *component)
{
	gtk_widget_destroy (component->frame);
	gtk_widget_destroy (component->fs);

	bonobo_object_destroy (BONOBO_OBJECT (component->client_site));

	g_free (component);
}

static void
component_view_frame_system_exception_cb (BonoboViewFrame *view_frame, CORBA_Object cobject,
					  CORBA_Environment *ev, gpointer data)
{
	bonobo_object_destroy (BONOBO_OBJECT (view_frame));
}

static void
component_add_view (Component *component)
{
	BonoboViewFrame *view_frame;
	GtkWidget       *view_widget;

	/*
	 * Create the remote view and the local ViewFrame.  This also
	 * sets the BonoboUIHandler for this ViewFrame.  That way, the
	 * embedded component can get access to our UIHandler server
	 * so that it can merge menu and toolbar items when it gets
	 * activated.
	 */
	view_frame = bonobo_client_site_new_view (
		component->client_site,
		bonobo_object_corba_objref (BONOBO_OBJECT (
			component->container->uih)));

	/*
	 * Connect to the "system_exception" signal on the
	 * newly-created view frame.  This signal will signify that
	 * the object has encountered a fatal CORBA exception and is
	 * defunct.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "system_exception",
			    component_view_frame_system_exception_cb, component);

	/*
	 * Embed the view frame into the application.
	 */
	view_widget = bonobo_view_frame_get_wrapper (view_frame);
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
			    GTK_SIGNAL_FUNC (component_user_activate_request_cb), component);

	/*
	 * In-place activation of a component is a two-step process.
	 * After the user double clicks on the component, our signal
	 * callback (component_user_activate_request_cb()) asks the
	 * component to activate itself (see
	 * bonobo_view_frame_view_activate()).  The component can then
	 * choose to either accept or refuse activation.  When an
	 * embedded component notifies us of its decision to change
	 * its activation state, the "view_activated" signal is
	 * emitted from the view frame.  It is at that point that we
	 * actually remove the cover so that events can get through.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "activated",
			    GTK_SIGNAL_FUNC (component_view_activated_cb), component);

	/*
	 * The "user_context" signal is emitted when the user right
	 * clicks on the wrapper.  We use it to pop up a verb menu.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "user_context",
			    GTK_SIGNAL_FUNC (component_user_context_cb), component);

	/*
	 * Show the component.
	 */
	gtk_widget_show_all (view_widget);
}


static void
component_new_view_cb (GtkWidget *button, Component *component)
{
	component_add_view (component);
}

static void
component_load_cancel_cb (GtkWidget *button, Component *component)
{
	gtk_widget_destroy (component->fs);
}

static void
component_create_fs (Component *component,
		     char *title, GtkSignalFunc ok_cb,
		     GtkSignalFunc cancel_cb)
{
	GtkWidget *fs;

	/*
	 * Create a file selection dialog.
	 */

	fs = gtk_file_selection_new (title);
	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fs));

	component->fs = fs;

	gtk_window_set_position (GTK_WINDOW (fs),
				 GTK_WIN_POS_MOUSE);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fs)->ok_button),
			    "clicked", ok_cb, component);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fs)->cancel_button),
			    "clicked", cancel_cb, component);

	gtk_window_set_modal (GTK_WINDOW (fs), TRUE);
	
	gtk_widget_show (fs);
}

static void
component_load_pf_ok_cb (GtkWidget *button, Component *component)
{
	Bonobo_PersistFile persist;
	CORBA_Environment ev;
	char *filename;

	/*
	 * First grab the filename.
	 */
	filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (component->fs));

	if (filename)
		filename = g_strdup (filename);

	/*
	 * Destroy the file selector.
	 */
	gtk_widget_destroy (component->fs);

	if (!filename)
		return;
	
	/*
	 * Now get the PersistFile interface off the embedded
	 * component.
	 */
	persist = bonobo_object_client_query_interface (component->server,
						       "IDL:Bonobo/PersistFile:1.0",
						       NULL);

	/*
	 * If the component doesn't support PersistFile (and it really
	 * ought to -- we query it to see if it supports PersistStream
	 * before we even give the user the option of loading data
	 * into it with PersistStream), then we destroy the stream we
	 * created and bail.
	 */
	if (persist == CORBA_OBJECT_NIL) {
		gnome_warning_dialog (_("The component now claims that it "
					"doesn't support PersistFile!"));
		g_free (filename);
		return;
	}

	CORBA_exception_init (&ev);

	/*
	 * Load the file into the component using PersistFilea.
	 */
	Bonobo_PersistFile_load (persist, (CORBA_char *) filename, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_warning_dialog (_("An exception occured while trying "
					"to load data into the component with "
					"PersistFile"));
	} else {
	  Bonobo_Unknown_unref (persist, &ev);
	
	  CORBA_Object_release (persist, &ev);
	}

	CORBA_exception_free (&ev);
	g_free (filename);
}

static void
component_load_pf_cb (GtkWidget *button, Component *component)
{
	component_create_fs (component,
			     _("Choose a file to load into the component using PersistFile"),
			     component_load_pf_ok_cb, component_load_cancel_cb);
}

/*
 * This callback is invoked when the user has selected a file to load
 * into a component using the PersistStream interface.  It must
 * retrieve the PersistStream interface from the component, load the
 * file into a stream, and pass the stream to the component's
 * PersistStream interface.
 */
static void
component_load_ps_ok_cb (GtkWidget *button, Component *component)
{
	Bonobo_PersistStream persist;
	CORBA_Environment ev;
	BonoboStream *stream;
	char *filename;

	/*
	 * First grab the filename and try to open the file.
	 */
	filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (component->fs));

	stream = bonobo_stream_fs_open (filename, Bonobo_Storage_READ);

	if (stream == NULL) {
		char *error_msg;

		error_msg = g_strdup_printf (_("Could not open file %s"), filename);
		gnome_warning_dialog (error_msg);
		g_free (error_msg);

		return;
	}

	/*
	 * Destroy the file selector.
	 */
	gtk_widget_destroy (component->fs);

	/*
	 * Now get the PersistStream interface off the embedded
	 * component.
	 */
	persist = bonobo_object_client_query_interface (component->server,
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
		gnome_warning_dialog (_("The component now claims that it "
					"doesn't support PersistStream!"));
		bonobo_object_unref (BONOBO_OBJECT (stream));
		return;
	}

	CORBA_exception_init (&ev);

	/*
	 * Load the file into the component using PersistStream.
	 */
	Bonobo_PersistStream_load (persist,
	  (Bonobo_Stream) bonobo_object_corba_objref (BONOBO_OBJECT (stream)),
				  &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		gnome_warning_dialog (_("An exception occured while trying "
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

/*
 * This callback is invoked when the user hits the 'Load data into
 * component using PersistStream' button.
 */
static void
component_load_ps_cb (GtkWidget *button, Component *component)
{
	component_create_fs (component, _("Choose a file to load into the component"),
			     component_load_ps_ok_cb, component_load_cancel_cb);
}

static void
component_destroy_cb (GtkWidget *button, Component *component)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_Unknown_unref (
		bonobo_object_corba_objref (BONOBO_OBJECT (component->server)),
		&ev);

	CORBA_exception_free (&ev);

	gtk_widget_destroy (component->frame);
}

static BonoboObjectClient *
container_launch_component (BonoboClientSite *client_site,
			    BonoboContainer *container,
			    char *component_goad_id)
{
	BonoboObjectClient *object_server;

	/*
	 * Launch the component.
	 */
	object_server = bonobo_object_activate_with_goad_id (
		NULL, component_goad_id, 0, NULL);

	if (object_server == NULL)
		return NULL;

	/*
	 * Bind it to the local ClientSite.  Every embedded component
	 * has a local BonoboClientSite object which serves as a
	 * container-side point of contact for the embeddable.  The
	 * container talks to the embeddable through its ClientSite
	 */
	if (! bonobo_client_site_bind_embeddable (client_site, object_server)) {
		bonobo_object_unref (BONOBO_OBJECT (object_server));
		return NULL;
	}

	/*
	 * The BonoboContainer object maintains a list of the
	 * ClientSites which it manages.  Here we add the new
	 * ClientSite to that list.
	 */
	bonobo_container_add (container, BONOBO_OBJECT (client_site));

	return object_server;
}

static void
container_create_component_frame (Container *container, Component *component, char *name)
{
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *button_hbox;

	/*
	 * The component, its views, and the buttons used to control
	 * it are stuffed into this GtkFrame.
	 */
	component->frame = gtk_frame_new (name);

	/*
	 * This vbox is used to separate the buttons from the views.
	 */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (component->frame), vbox);

	/*
	 * This hbox will store all the views of the component.
	 */
	component->views_hbox = gtk_hbox_new (FALSE, 2);

	/*
	 * All the buttons go into this hbox.
	 */
	button_hbox = gtk_hbox_new (FALSE, 0);


	gtk_box_pack_start (GTK_BOX (vbox), component->views_hbox,
			    FALSE, FALSE, 5);
	gtk_box_pack_end (GTK_BOX (vbox) , button_hbox,
			  FALSE, FALSE, 5);

	/*
	 * Create the 'New View' button.
	 */
	button = gtk_button_new_with_label (_("New View"));

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (component_new_view_cb),
			    component);

	gtk_box_pack_start (GTK_BOX (button_hbox), button, TRUE, FALSE, 0);

	/*
	 * Create the 'Load component with PersistFile' button.
	 */
	if (bonobo_object_client_has_interface (component->server,
					       "IDL:Bonobo/PersistFile:1.0",
					       NULL)) {
		
		button = gtk_button_new_with_label (_("Load with PersistFile"));

		gtk_box_pack_start (GTK_BOX (button_hbox), button,
				    TRUE, FALSE, 0);

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    GTK_SIGNAL_FUNC (component_load_pf_cb),
				    component);
	}

	/*
	 * Create the 'Load component with PersistStream' button.
	 */
	if (bonobo_object_client_has_interface (component->server,
			"IDL:Bonobo/PersistStream:1.0", NULL)) {
		button = gtk_button_new_with_label (_("Load with PersistStream"));

		gtk_box_pack_start (GTK_BOX (button_hbox), button,
				    TRUE, FALSE, 0);

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    GTK_SIGNAL_FUNC (component_load_ps_cb),
				    component);
	}

	/*
	 * Create the 'destroy component' button.
	 */
	button = gtk_button_new_with_label (_("Destroy component"));

	gtk_box_pack_start (GTK_BOX (button_hbox), button,
			    TRUE, FALSE, 0);

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (component_destroy_cb),
			    component);

	/*
	 * Stuff the frame into the container's vbox.
	 */
	gtk_box_pack_start (GTK_BOX (container->vbox), component->frame,
			    TRUE, FALSE, 5);
	gtk_widget_show_all (component->frame);
}

static void
component_client_site_system_exception_cb (BonoboObject *client, CORBA_Object cobject,
					   CORBA_Environment *ev, gpointer data)
{
	Component *component = (Component *) data;

	component_shutdown (component);
}

static void
container_activate_component (Container *container, char *component_goad_id)
{
	Component *component;
	BonoboClientSite *client_site;
	BonoboObjectClient *server;

	/*
	 * The ClientSite is the container-side point of contact for
	 * the Embeddable.  So there is a one-to-one correspondence
	 * between BonoboClientSites and BonoboEmbeddables.
	 */
	client_site = bonobo_client_site_new (container->container);

	/*
	 * A BonoboObjectClient is a simple wrapper for a remote
	 * BonoboObject (a server supporting Bonobo::Unknown).
	 */
	server = container_launch_component (client_site, container->container,
					     component_goad_id);
	if (server == NULL) {
		char *error_msg;

		error_msg = g_strdup_printf (_("Could not launch Embeddable %s!"),
					     component_goad_id);
		gnome_warning_dialog (error_msg);
		g_free (error_msg);

		return;
	}

	/*
	 * Create the internal data structure which we will use to
	 * keep track of this component.
	 */
	component = g_new0 (Component, 1);
	component->container = container;
	component->client_site = client_site;
	component->server = server;

	/*
	 * Now we have a BonoboEmbeddable bound to our local
	 * ClientSite.  Here we create a little on-screen box to store
	 * the embeddable in, when the user adds views for it.
	 */
	container_create_component_frame (container, component, component_goad_id);

	/*
	 * Connect to the "system_exception" signal so that we can
	 * shut down this ClientSite when it encounters a fatal CORBA
	 * exception.
	 */
	gtk_signal_connect (GTK_OBJECT (client_site), "system_exception",
			    component_client_site_system_exception_cb,
			    component);
}

static void
container_add_embeddable_cmd (GtkWidget *widget, Container *container)
{
	char *required_interfaces[2] = { "IDL:Bonobo/Embeddable:1.0", NULL };
	char *goad_id;

	/*
	 * Ask the user to select a component.
	 */
	goad_id = gnome_bonobo_select_goad_id (
		_("Select an embeddable Bonobo component to add"),
		(const gchar **) required_interfaces);

	if (goad_id == NULL)
		return;

	/*
	 * Activate it.
	 */
	container_activate_component (container, goad_id);

	g_free (goad_id);
}

static void
container_print_preview_cmd (GtkWidget *widget, Container *container)
{
	GList *l;
	GnomePrintMaster *pm;
	GnomePrintContext *ctx;
	GnomePrintMasterPreview *pv;
	double ypos = 0.0;

	g_return_if_fail (container != NULL);
	g_return_if_fail (container->container != NULL);

	pm = gnome_print_master_new ();
	ctx = gnome_print_master_get_context (pm);

	for (l = container->container->client_sites; l; l = l->next) {
		BonoboClientSite   *cs = l->data;
		BonoboObjectClient *boc = bonobo_client_site_get_embeddable (cs);
		BonoboPrintClient  *pc = bonobo_print_client_get (boc);
		BonoboPrintContext *c;

		if (!pc) {
			g_warning ("component isn't printable");
			continue;
		} else
			g_warning ("component is printable!");

		c = bonobo_print_context_new (0.0, ypos, 100.0, 150.0);
		bonobo_print_client_print_to (pc, c, ctx);
		bonobo_print_context_free (c);
		ypos += 150.0;
	}

	gnome_print_context_close (ctx);
	pv = gnome_print_master_preview_new (pm, "Component demo");
	gtk_widget_show (GTK_WIDGET (pv));
	gtk_main ();
	gtk_object_unref (GTK_OBJECT (pv));
	gtk_object_unref (GTK_OBJECT (pm));
}

static void
container_create_menus (Container *container)
{
	BonoboUIHandlerMenuItem *menu_list;

	bonobo_ui_handler_create_menubar (container->uih);

	/*
	 * Create the basic menus out of UIInfo structures.
	 */
	menu_list = bonobo_ui_handler_menu_parse_uiinfo_list_with_data (container_main_menu, container);
	bonobo_ui_handler_menu_add_list (container->uih, "/", menu_list);
	bonobo_ui_handler_menu_free_list (menu_list);
}

/*
 * This callback is invoked if the BonoboContainer raises a
 * system_exception signal, signifying that a fatal CORBA exception
 * has been encountered.
 */
static void
container_container_system_exception_cb (BonoboObject *container_object, CORBA_Object cobject,
					 CORBA_Environment *ev, gpointer data)
{
	Container *container = (Container *) data;

	gnome_warning_dialog (_("Container encountered a fatal CORBA exception!  Shutting down..."));

	bonobo_object_destroy (BONOBO_OBJECT (container->container));
	gtk_widget_destroy (container->app);

	gtk_main_quit ();
}

static void
container_create (void)
{
	Container *container;

	container = g_new0 (Container, 1);

	container->app = gnome_app_new ("sample-container",
					"Sample Bonobo Subdocument Container");

	gtk_window_set_default_size (GTK_WINDOW (container->app), 400, 400);
	gtk_window_set_policy (GTK_WINDOW (container->app), TRUE, TRUE, FALSE);

	container->container = bonobo_container_new ();

	/*
	 * The "system_exception" signal will notify us if a fatal CORBA
	 * exception has rendered the BonoboContainer defunct.
	 */
	gtk_signal_connect (GTK_OBJECT (container->container), "system_exception",
			    container_container_system_exception_cb, container);

	/*
	 * This is the VBox we will stuff embedded components into.
	 */
	container->vbox = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (container->app), container->vbox);

	/*
	 * Create the BonoboUIHandler object which will be used to
	 * create the container's menus and toolbars.  The UIHandler
	 * also creates a CORBA server which embedded components use
	 * to do menu/toolbar merging.
	 */
	container->uih = bonobo_ui_handler_new ();
	bonobo_ui_handler_set_app (container->uih, GNOME_APP (container->app));

	/*
	 * Create the menus.
	 */
	container_create_menus (container);
	
	gtk_widget_show_all (container->app);
}

int
main (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_ORB orb;

	CORBA_exception_init (&ev);

	gnome_CORBA_init ("sample-container", "1.0", &argc, argv, 0, &ev);

	CORBA_exception_free (&ev);

	orb = gnome_CORBA_ORB ();

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	container_create ();

	bonobo_main ();

	return 0;
}
