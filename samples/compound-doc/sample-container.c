/*
 * sample-container.c
 *
 * FIXME: Description.
 *
 * Author:
 *   Nat Friedman (nat@nat.org)
 */

#include <config.h>
#include <gnome.h>
#include <bonobo/gnome-bonobo.h>

typedef struct {
	GnomeContainer  *container;
	GnomeUIHandler  *uih;

	GnomeViewFrame  *active_view_frame;

	GtkWidget	*app;
	GtkWidget	*vbox;
} Container;

typedef struct {
	Container	  *container;

	GnomeClientSite   *client_site;
	GnomeViewFrame	  *view_frame;
	GnomeObjectClient *server;

	GtkWidget	  *frame;
	GtkWidget	  *views_hbox;
} Component;

/*
 * Static prototypes.
 */
static void container_add_embeddable_cmd (GtkWidget *widget, Container *container);
static void container_exit_cmd (GtkWidget *widget, Container *container); 

/*
 * The menus.
 */
static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("_Add a new Embeddable component"), NULL,
		container_add_embeddable_cmd),
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
	 * FIXME: Kill all components.
	 */
	gtk_main_quit ();
}

static void
component_user_activate_request_cb (GnomeViewFrame *view_frame, gpointer data)
{
	Component *component = (Component *) data;
	Container *container = component->container;

	/*
	 * If there is a
	 * If there is already an active View, deactivate it.
	 */
        if (container->active_view_frame != NULL) {
		/*
		 * This just sends a notice to the embedded View that
		 * it is being deactivated.  We will also forcibly
		 * cover it so that it does not receive any Gtk
		 * events.
		 */
                gnome_view_frame_view_deactivate (container->active_view_frame);

		/*
		 * Here we manually cover it if it hasn't acquiesced.
		 * If it has consented to be deactivated, then it will
		 * already have notified us that it is inactive, and
		 * we will have covered it and set active_view_frame
		 * to NULL.  Which is why this check is here.
		 */
		if (container->active_view_frame != NULL)
			gnome_view_frame_set_covered (container->active_view_frame, TRUE);
									     
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
        gnome_view_frame_view_activate (view_frame);
}

static void
component_view_activated_cb (GnomeViewFrame *view_frame, gboolean activated, gpointer data)
{
	Component *component = (Component *) data;
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
		gnome_view_frame_set_covered (view_frame, FALSE);
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
		gnome_view_frame_set_covered (view_frame, TRUE);

		if (view_frame == container->active_view_frame)
			container->active_view_frame = NULL;
        }                                                                       
}

static void
component_user_context_cb (GnomeViewFrame *view_frame, gpointer data)
{
	Component *component = (Component *) data;
	char *executed_verb;
	GList *l;

	/*
	 * See if the remote GnomeEmbeddable supports any verbs at
	 * all.
	 */
	l = gnome_client_site_get_verbs (component->client_site);
	if (l == NULL)
		return;
	gnome_client_site_free_verbs (l);

	/*
	 * Popup the verb popup and execute the chosen verb.  This
	 * function saves us the work of creating the menu, connecting
	 * the callback, and executing the verb on the remove
	 * GnomeView.  We could implement all this functionality
	 * ourselves if we wanted.
	 */
	executed_verb = gnome_view_frame_popup_verbs (view_frame);

	g_free (executed_verb);
}

static void
component_add_view (Component *component)
{
	GnomeViewFrame *view_frame;
	GtkWidget *view_widget;

	/*
	 * Create the remote view and the local ViewFrame.
	 */
	view_frame = gnome_client_site_embeddable_new_view (component->client_site);
	component->view_frame = view_frame;

	/*
	 * Set the GnomeUIHandler for this ViewFrame.  That way, the
	 * embedded component can get access to our UIHandler server
	 * so that it can merge menu and toolbar items when it gets
	 * activated.
	 */
	gnome_view_frame_set_ui_handler (view_frame, component->container->uih);

	/*
	 * Embed the view frame into the application.
	 */
	view_widget = gnome_view_frame_get_wrapper (view_frame);
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
	 * callback (compoennt_user_activate_request_cb()) asks the
	 * component to activate itself (see
	 * gnome_view_frame_view_activate()).  The component can then
	 * choose to either accept or refuse activation.  When an
	 * embedded component notifies us of its decision to change
	 * its activation state, the "view_activated" signal is
	 * emitted from the view frame.  It is at that point that we
	 * actually remove the cover so that events can get through.
	 */
	gtk_signal_connect (GTK_OBJECT (view_frame), "view_activated",
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
component_new_view_cb (GtkWidget *button, gpointer data)
{
	Component *component = (Component *) data;

	component_add_view (component);
}

static void
component_load_pf_cb (GtkWidget *button, gpointer data)
{
}

static void
component_load_ps_cb (GtkWidget *button, gpointer data)
{
}

static void
component_destroy_cb (GtkWidget *button, gpointer data)
{
}

static GnomeObjectClient *
container_launch_component (GnomeClientSite *client_site,
			    GnomeContainer *container,
			    char *component_goad_id)
{
	GnomeObjectClient *object_server;

	/*
	 * Launch the component.
	 */
	object_server = gnome_object_activate_with_goad_id (
		NULL, component_goad_id, 0, NULL);

	if (object_server == NULL)
		return NULL;

	/*
	 * Bind it to the local ClientSite.  Every embedded component
	 * has a local GnomeClientSite object which serves as a
	 * container-side point of contact for the embeddable.  The
	 * container talks to the embeddable through its ClientSite
	 */
	if (! gnome_client_site_bind_embeddable (client_site, object_server)) {
		gnome_object_unref (GNOME_OBJECT (object_server));
		return NULL;
	}

	/*
	 * The GnomeContainer object maintains a list of the
	 * ClientSites which it manages.  Here we add the new
	 * ClientSite to that list.
	 */
	gnome_container_add (container, GNOME_OBJECT (client_site));

	return object_server;
}

/*
 * Use query_interface to see if `obj' has `interface'.
 */
static gboolean
gnome_object_has_interface (GnomeObject *obj, char *interface)
{
	CORBA_Environment ev;
	CORBA_Object requested_interface;

	CORBA_exception_init (&ev);

	requested_interface = GNOME_Unknown_query_interface (
		gnome_object_corba_objref (obj), interface, &ev);

	CORBA_exception_free (&ev);

	if (!CORBA_Object_is_nil(requested_interface, &ev) &&
	    ev._major == CORBA_NO_EXCEPTION)
	{
		/* Get rid of the interface we've been passed */
		CORBA_Object_release (requested_interface, &ev);
		return TRUE;
	}

	return FALSE;
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
	if (gnome_object_has_interface (GNOME_OBJECT (component->server),
					"IDL:GNOME/PersistFile:1.0")) {

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
	if (gnome_object_has_interface (GNOME_OBJECT (component->server),
					"IDL:GNOME/PersistStream:1.0")) {
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
container_activate_component (Container *container, char *component_goad_id)
{
	Component *component;
	GnomeClientSite *client_site;
	GnomeObjectClient *server;

	/*
	 * The ClientSite is the container-side point of contact for
	 * the Embeddable.  So there is a one-to-one correspondence
	 * between GnomeClientSites and GnomeEmbeddables.  */
	client_site = gnome_client_site_new (container->container);

	/*
	 * A GnomeObjectClient is a simple wrapper for a remote
	 * GnomeObject (a server supporting GNOME::Unknown).
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
	 * Now we have a GnomeEmbeddable bound to our local
	 * ClientSite.  Here we create a little on-screen box to store
	 * the embeddable in, when the user adds views for it.
	 */
	container_create_component_frame (container, component, component_goad_id);
}

static void
container_add_embeddable_cmd (GtkWidget *widget, Container *container)
{
	char *required_interfaces[2];
	char *goad_id;

	/*
	 * Ask the user to select a component.
	 */
	required_interfaces[0] = "IDL:GNOME/Embeddable:1.0";
	required_interfaces[1] = NULL;
	goad_id = gnome_bonobo_select_goad_id (
		_("Select an embeddable Bonobo component to add"),
		(const gchar **) required_interfaces);

	/*
	 * Activate it.
	 */
	container_activate_component (container, goad_id);

	g_free (goad_id);
}

static void
container_create_menus (Container *container)
{
	GnomeUIHandlerMenuItem *menu_list;

	gnome_ui_handler_create_menubar (container->uih);

	/*
	 * Create the basic menus out of UIInfo structures.
	 */
	menu_list = gnome_ui_handler_menu_parse_uiinfo_list_with_data (container_main_menu, container);
	gnome_ui_handler_menu_add_list (container->uih, "/", menu_list);
	gnome_ui_handler_menu_free_list (menu_list);
}

static void
container_create (void)
{
	Container *container;

	container = g_new0 (Container, 1);

	container->app = gnome_app_new ("sample-container",
					"Sample Bonobo Container");

	gtk_window_set_default_size (GTK_WINDOW (container->app), 400, 400);
	gtk_window_set_policy (GTK_WINDOW (container->app), TRUE, TRUE, FALSE);

	container->container = gnome_container_new ();

	/*
	 * This is the VBox we will stuff embedded components into.
	 */
	container->vbox = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (container->app), container->vbox);

	/*
	 * Create the GnomeUIHandler object which will be used to
	 * create the container's menus and toolbars.  The UIHandler
	 * also creates a CORBA server which embedded components use
	 * to do menu/toolbar merging.
	 */
	container->uih = gnome_ui_handler_new ();
	gnome_ui_handler_set_app (container->uih, GNOME_APP (container->app));

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

	gtk_main ();

	return 0;
}
