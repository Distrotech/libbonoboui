/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * test-container.c
 *
 * A simple program to act as a test container for embeddable
 * components.
 *
 * Authors:
 *    Nat Friedman (nat@nat.org)
 *    Miguel de Icaza (miguel@gnu.org)
 */
 
#include <config.h>
#include <gnome.h>
#include <liboaf/liboaf.h>

#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo.h>
#include <sys/stat.h>
#include <unistd.h>

CORBA_Environment ev;
CORBA_ORB orb;

/*
 * A handle to some Embeddables and their ClientSites so we can add
 * views to existing components.
 */
BonoboObjectClient *text_obj;
BonoboClientSite   *text_client_site;

BonoboObjectClient *image_png_obj;
BonoboClientSite   *image_client_site;

BonoboObjectClient *paint_obj;
BonoboClientSite   *paint_client_site;

/*
 * The currently active view.  We keep track of this
 * so we can deactivate it when a new view is activated.
 */
BonoboViewFrame *active_view_frame;

char *server_id = "OAFIID:test_bonobo_object:b1ff15bb-d54f-4814-ba53-d67d3afd70fe";

typedef struct {
	GtkWidget *app;
	BonoboContainer *container;
	GtkWidget *box;
	Bonobo_View view;
	BonoboUIHandler *uih;
} Application;

static BonoboObjectClient *
launch_server (BonoboClientSite *client_site, BonoboContainer *container, char *id)
{
	BonoboObjectClient *object_server;
	
	bonobo_container_add (container, BONOBO_OBJECT (client_site));

	printf ("Launching...\n");
	object_server = bonobo_object_activate (id, 0);
	printf ("Return: %p\n", object_server);
	if (!object_server){
		g_warning (_("Can not activate object_server"));
		return NULL;
	}

	if (!bonobo_client_site_bind_embeddable (client_site, object_server)){
		g_warning (_("Can not bind object server to client_site"));
		return NULL;
	}

	return object_server;
}

static BonoboObjectClient *
launch_server_moniker (BonoboClientSite  *client_site,
		       BonoboContainer   *container,
		       Bonobo_Moniker     moniker,
		       CORBA_Environment *ev)
{
	char *moniker_name;
	BonoboObjectClient *object_server;
	
	bonobo_container_add (container, BONOBO_OBJECT (client_site));

	moniker_name = bonobo_moniker_client_get_name (moniker, ev);
	printf ("Launching moniker '%s' ...\n", moniker_name);
	CORBA_free (moniker_name);

	object_server = bonobo_moniker_client_resolve_client_default (
		moniker, "IDL:Bonobo/Embeddable:1.0", ev);

	printf ("Return: %p\n", object_server);
	if (!object_server) {
		g_warning (_("Can not activate object_server"));
		return NULL;
	}

	if (!bonobo_client_site_bind_embeddable (client_site, object_server)) {
		g_warning (_("Can not bind object server to client_site"));
		return NULL;
	}

	return object_server;
}

/*
 * This function is called when the user double clicks on a View in
 * order to activate it.
 */
static gint
user_activation_request_cb (BonoboViewFrame *view_frame)
{
	/*
	 * If there is already an active View, deactivate it.
	 */
        if (active_view_frame != NULL) {
		/*
		 * This just sends a notice to the embedded View that
		 * it is being deactivated.  We will also forcibly
		 * cover it so that it does not receive any Gtk
		 * events.
		 */
                bonobo_view_frame_view_deactivate (active_view_frame);

		/*
		 * Here we manually cover it if it hasn't acquiesced.
		 * If it has consented to be deactivated, then it will
		 * already have notified us that it is inactive, and
		 * we will have covered it and set active_view_frame
		 * to NULL.  Which is why this check is here.
		 */
		if (active_view_frame != NULL)
			bonobo_view_frame_set_covered (active_view_frame, TRUE);
									     
		active_view_frame = NULL;
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

        return FALSE;
}                                                                               

/*
 * Gets called when the View notifies the ViewFrame that it would like
 * to be activated or deactivated.
 */
static gint
view_activated_cb (BonoboViewFrame *view_frame, gboolean activated)
{

        if (activated) {
		/*
		 * If the View is requesting to be activated, then we
		 * check whether or not there is already an active
		 * View.
		 */
		if (active_view_frame != NULL) {
			g_warning ("View requested to be activated but there is already "
				   "an active View!");
			return FALSE;
		}

		/*
		 * Otherwise, uncover it so that it can receive
		 * events, and set it as the active View.
		 */
		bonobo_view_frame_set_covered (view_frame, FALSE);
                active_view_frame = view_frame;
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

		if (view_frame == active_view_frame)
			active_view_frame = NULL;
        }                                                                       

        return FALSE;
}                                                                               

static BonoboViewFrame *
add_view (GtkWidget *widget, Application *app,
	  BonoboClientSite *client_site, BonoboObjectClient *server) 
{
	BonoboViewFrame *view_frame;
	GtkWidget *view_widget;
	GtkWidget *frame;
	
	view_frame = bonobo_client_site_new_view (client_site, CORBA_OBJECT_NIL);

	gtk_signal_connect (GTK_OBJECT (view_frame), "user_activate",
			    GTK_SIGNAL_FUNC (user_activation_request_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (view_frame), "view_activated",
			    GTK_SIGNAL_FUNC (view_activated_cb), NULL);

	view_widget = bonobo_view_frame_get_wrapper (view_frame);

	frame = gtk_frame_new ("Embeddable");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (app->box), frame, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), view_widget);

	gtk_widget_show_all (frame);

	return view_frame;
} /* add_view */

static BonoboObjectClient *
add_cmd (GtkWidget *widget, Application *app, char *server_id,
	 BonoboClientSite **client_site)
{
	BonoboObjectClient *server;
	
	*client_site = bonobo_client_site_new (app->container);

	server = launch_server (*client_site, app->container, server_id);
	if (server == NULL)
		return NULL;

	add_view (widget, app, *client_site, server);
	return server;
}

static BonoboObjectClient *
add_cmd_moniker (GtkWidget *widget, Application *app,
		 Bonobo_Moniker moniker, BonoboClientSite **client_site,
		 CORBA_Environment *ev)
{
	BonoboObjectClient *server;
	
	*client_site = bonobo_client_site_new (app->container);

	server = launch_server_moniker (*client_site, app->container,
					moniker, ev);
	if (server == NULL)
		return NULL;

	add_view (widget, app, *client_site, server);
	return server;
}

static void
add_demo_cmd (GtkWidget *widget, Application *app)
{
	BonoboClientSite *client_site;
	add_cmd (widget, app, server_id, &client_site);
}

static void
add_image_cmd (GtkWidget *widget, Application *app)
{
	BonoboObjectClient *object;
	BonoboStream *stream;
	Bonobo_PersistStream persist;

	object = add_cmd (widget, app, "OAFIID:bonobo_image-x-png:716e8910-656b-4b3b-b5cd-5eda48b71a79",
			  &image_client_site);


	if (object == NULL) {
		gnome_warning_dialog (_("Could not launch bonobo object."));
		return;
	}

	image_png_obj = object;

	persist = bonobo_object_client_query_interface (
		object, "IDL:Bonobo/PersistStream:1.0", NULL);

        if (persist == CORBA_OBJECT_NIL) {
		printf ("No persist-stream interface\n");
                return;
	}

	printf ("Good: Embeddable supports PersistStream\n");
	
	stream = bonobo_stream_fs_open ("/tmp/a.png", Bonobo_Storage_READ);

	if (stream == NULL) {
		printf ("I could not open /tmp/a.png!\n");
		return;
	}
	
	Bonobo_PersistStream_load (
		persist,
		(Bonobo_Stream) bonobo_object_corba_objref (BONOBO_OBJECT (stream)), "", &ev);

	Bonobo_Unknown_unref  (persist, &ev);
	CORBA_Object_release (persist, &ev);
}

static void
add_pdf_cmd (GtkWidget *widget, Application *app)
{
	BonoboObjectClient *object;
	BonoboStream *stream;
	Bonobo_PersistStream persist;

	g_error ("Wrong oafid for pdf");
	object = add_cmd (widget, app, "bonobo-object:application-x-pdf", &image_client_site);

	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch bonobo object."));
	    return;
	  }

	image_png_obj = object;

	persist = bonobo_object_client_query_interface (
		object, "IDL:Bonobo/PersistStream:1.0", NULL);

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports PersistStream\n");
	
	stream = bonobo_stream_fs_open ("/tmp/a.pdf", Bonobo_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /tmp/a.pdf!\n");
		return;
	}
	
	Bonobo_PersistStream_load (
		persist,
		(Bonobo_Stream) bonobo_object_corba_objref (BONOBO_OBJECT (stream)), "", &ev);

	Bonobo_Unknown_unref (persist, &ev);
	CORBA_Object_release (persist, &ev);
}

/*
 * Add a new view for the existing application/x-png Embeddable.
 */
static void
add_image_view (GtkWidget *widget, Application *app)
{
	if (image_png_obj == NULL)
		return;

	add_view (NULL, app, image_client_site, image_png_obj);
} /* add_image_view */

static void
add_gnumeric_cmd (GtkWidget *widget, Application *app)
{
	BonoboClientSite *client_site;
	Bonobo_Moniker    moniker;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	moniker = bonobo_moniker_client_new_from_name (
		"file:/tmp/sales.gnumeric!Sheet1!A1:D1", &ev);

	add_cmd_moniker (widget, app, moniker, &client_site, &ev); 

	bonobo_object_release_unref (moniker, &ev);

	CORBA_exception_free (&ev);
}

static int
item_event_handler (GnomeCanvasItem *item, GdkEvent *event)
{
	static double last_x, last_y;
	static int pressed;
	double delta_x, delta_y;
	
	switch (event->type){
	case GDK_BUTTON_PRESS:
		pressed = 1;
		last_x = event->button.x;
		last_y = event->button.y;
		printf ("Evento: %g %g\n", last_x, last_y);
		break;

	case GDK_MOTION_NOTIFY:
		if (!pressed)
			return FALSE;
		
		delta_x = event->motion.x - last_x;
		delta_y = event->motion.y - last_y;
		gnome_canvas_item_move (item, delta_x, delta_y);
		printf ("Motion: %g %g\n", delta_x, delta_y);
		last_x = event->motion.x;
		last_y = event->motion.y;
		break;

	case GDK_BUTTON_RELEASE:
		pressed = 0;
		break;
		
	default:
		return FALSE;
	}
	return TRUE;
}

static void
do_add_canvas_cmd (GtkWidget *widget, Application *app, gboolean aa)
{
	BonoboClientSite *client_site;
	GtkWidget *canvas, *frame, *sw;
	CORBA_Environment ev;
	BonoboObjectClient *server;
	GnomeCanvasItem *item;
	
	client_site = bonobo_client_site_new (app->container);

	server = launch_server (
		client_site, app->container,
		"OAFIID:test_canvas_item:82a8a7cc-8b08-401b-9501-4debf6c96619");

	if (server == NULL) {
		g_warning ("Can not activate OAFIID:test_canvas_item:82a8a7cc-8b08-401b-9501-4debf6c96619");
		return;
	}
	CORBA_exception_init (&ev);

	/*
	 * Setup our demostration canvas
	 */
	sw = gtk_scrolled_window_new (NULL, NULL);
	if (aa) {
		gtk_widget_push_visual (gdk_rgb_get_visual ());
		gtk_widget_push_colormap (gdk_rgb_get_cmap ());
		canvas = gnome_canvas_new_aa ();
		gtk_widget_pop_visual ();
		gtk_widget_pop_colormap ();
	} else
		canvas = gnome_canvas_new ();
	
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), -100, -100, 200, 200);
	gtk_widget_set_usize (canvas, 100, 100);

	/*
	 * Add a background
	 */
	gnome_canvas_item_new (
		GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (canvas))),
		gnome_canvas_rect_get_type (),
		"x1", 0.0,
		"y1", 0.0,
		"x2", 100.0,
		"y2", 100.0,
		"fill_color", "red",
		"outline_color", "blue",
		"width_pixels", 8,
		NULL);

	/*
	 * The remote item
	 */
	item = bonobo_client_site_new_item (
		BONOBO_CLIENT_SITE (client_site),
		GNOME_CANVAS_GROUP (gnome_canvas_root (GNOME_CANVAS (canvas))));

	gtk_signal_connect (
		GTK_OBJECT (item), "event",
		GTK_SIGNAL_FUNC (item_event_handler), NULL);
	
	frame = gtk_frame_new ("Canvas with a remote item");
	gtk_box_pack_start (GTK_BOX (app->box), frame, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), sw);
	gtk_container_add (GTK_CONTAINER (sw), canvas);
	gtk_widget_show_all (frame);
}

static void
add_canvas_cmd (GtkWidget *widget, Application *app)
{
	do_add_canvas_cmd (widget, app, FALSE);
}

static void
add_canvas_aa_cmd (GtkWidget *widget, Application *app)
{
	do_add_canvas_cmd (widget, app, TRUE);
}

static void
add_paint_cmd (GtkWidget *widget, Application *app)
{
	BonoboObjectClient *object;

	object = add_cmd (widget, app, 
			  "OAFIID:paint_component_simple:9c04da1c-d44c-4041-9991-fed1ed1ed079",
			  &paint_client_site);

	if (object == NULL)
	  {
	    gnome_warning_dialog (_("Could not launch Embeddable."));
	    return;
	  }

	paint_obj = object;
}

static void
add_paint_view (GtkWidget *widget, Application *app)
{
	if (paint_obj == NULL)
		return;

	add_view (NULL, app, paint_client_site, paint_obj);
}

/*
 * This function uses Bonobo::PersistStream to load a set of data into
 * the text/plain Embeddable.
 */
static void
add_text_cmd (GtkWidget *widget, Application *app)
{
	BonoboObjectClient *object;
	BonoboStream *stream;
	Bonobo_PersistStream persist;

	object = add_cmd (widget, app,
			  "OAFIID:bonobo_text-plain:26e1f6ba-90dd-4783-b304-6122c4b6c821",
			  &text_client_site);

	if (object == NULL) {
		gnome_warning_dialog (_("Could not launch Embeddable."));
		return;
	}

	text_obj = object;

	persist = bonobo_object_client_query_interface (object,
		"IDL:Bonobo/PersistStream:1.0", &ev);

        if (ev._major != CORBA_NO_EXCEPTION)
                return;

        if (persist == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports PersistStream\n");
	
	stream = bonobo_stream_fs_open ("/etc/passwd", Bonobo_Storage_READ);

	if (stream == NULL){
		printf ("I could not open /etc/passwd!\n");
		return;
	}
	
	Bonobo_PersistStream_load (
	     persist, (Bonobo_Stream) bonobo_object_corba_objref (BONOBO_OBJECT (stream)), "", &ev);

	Bonobo_Unknown_unref (persist, &ev);
	CORBA_Object_release (persist, &ev);
} /* add_text_cmd */

/*
 * These functions handle the progressive transmission of data
 * to the text/plain Embeddable.
 */
struct progressive_timeout {
	Bonobo_ProgressiveDataSink psink;
	FILE *f;
};

/*
 * Send a new line to the text/plain Embeddable.
 */
static gboolean
timeout_next_line (gpointer data)
{
	struct progressive_timeout *tmt = (struct progressive_timeout *) data;
  
	Bonobo_ProgressiveDataSink_iobuf *buffer;
	char line[1024];
	int line_len;
	
	if (fgets (line, sizeof (line), tmt->f) == NULL)
	{
		Bonobo_ProgressiveDataSink_end (tmt->psink, &ev);

		fclose (tmt->f);

		Bonobo_Unknown_unref (tmt->psink, &ev);
		CORBA_Object_release (tmt->psink, &ev);

		g_free (tmt);
		return FALSE;
	}

	line_len = strlen (line);

	buffer = Bonobo_ProgressiveDataSink_iobuf__alloc ();
	CORBA_sequence_set_release (buffer, TRUE);

	buffer->_length = line_len;
	buffer->_buffer = CORBA_sequence_CORBA_octet_allocbuf (line_len);
	memcpy (buffer->_buffer, line, line_len);

	Bonobo_ProgressiveDataSink_add_data (tmt->psink, buffer, &ev);

	return TRUE;
} /* timeout_add_more_data */

/*
 * Add a new view for the existing text Embeddable.
 */
static void
add_text_view (GtkWidget *widget, Application *app)
{
	if (text_obj == NULL)
		return;

	add_view (NULL, app, text_client_site, text_obj);
} /* add_text_view */

/*
 * Setup a timer to send a new line to the text/plain Embeddable using
 * ProgressiveDataSink.
 */
static void
send_text_cmd (GtkWidget *widget, Application *app)
{
	Bonobo_ProgressiveDataSink psink;
	struct progressive_timeout *tmt;
	struct stat statbuf;
	FILE *f;

	if (text_obj == NULL)
		return;

	psink = bonobo_object_client_query_interface (text_obj,
                 "IDL:Bonobo/ProgressiveDataSink:1.0", NULL);

        if (psink == CORBA_OBJECT_NIL)
                return;

	printf ("Good: Embeddable supports ProgressiveDataSink\n");

	tmt = g_new0 (struct progressive_timeout, 1);

	Bonobo_ProgressiveDataSink_start (psink, &ev);

	f = fopen ("/etc/passwd", "r");
	if (f == NULL) {
		printf ("I could not open /etc/passwd!\n");
		return;
	}
	
	fstat (fileno (f), &statbuf);
	Bonobo_ProgressiveDataSink_set_size (psink,
					    (CORBA_long) statbuf.st_size,
					    &ev);

	tmt->psink = psink;
	tmt->f = f;

	g_timeout_add (500, timeout_next_line, (gpointer) tmt);
} /* send_text_cmd */

static void
exit_cmd (void)
{
	gtk_main_quit ();
}

static GnomeUIInfo container_text_plain_menu [] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Add a new text/plain component"), NULL,
			       add_text_cmd),
	GNOMEUIINFO_ITEM_NONE (
	N_("_Send progressive data to an existing text/plain component"),
			       NULL, send_text_cmd),
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new _view to an existing text/plain component"),
			       NULL, add_text_view),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_paint_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("_Add a new simple paint component"), NULL,
		add_paint_cmd),
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new _view to an existing paint component"), NULL,
		add_paint_view),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_image_png_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("_Add a new application/x-png component"), NULL,
		add_image_cmd),
 	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new _view to an existing application/x-png component"),
			       NULL, add_image_view),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_image_pdf_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("_Add a new application/x-pdf component"), NULL,
		add_pdf_cmd),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_gnumeric_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new Gnumeric instance through monikers"),
		NULL, add_gnumeric_cmd),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_file_menu [] = {
	GNOMEUIINFO_ITEM_NONE(N_("Add a new _object"), NULL, add_demo_cmd),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cmd, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_canvas_menu [] = {
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new Sample-Canvas item on an AA canvas"),
		NULL, add_canvas_aa_cmd),
	GNOMEUIINFO_ITEM_NONE (
		N_("Add a new Sample-Canvas item on a regular canvas"),
		NULL, add_canvas_cmd),
	GNOMEUIINFO_END
};

static GnomeUIInfo container_main_menu [] = {
	GNOMEUIINFO_MENU_FILE_TREE (container_file_menu),
	GNOMEUIINFO_SUBTREE (N_("_text/plain"), container_text_plain_menu),
	GNOMEUIINFO_SUBTREE (N_("_image/x-png"), container_image_png_menu),
	GNOMEUIINFO_SUBTREE (N_("_app/x-pdf"), container_image_pdf_menu),
	GNOMEUIINFO_SUBTREE (N_("paint sample"), container_paint_menu),
	GNOMEUIINFO_SUBTREE (N_("Gnumeric"), container_gnumeric_menu),
	GNOMEUIINFO_SUBTREE (N_("Canvas-based"), container_canvas_menu),
	GNOMEUIINFO_END
};

static Application *
application_new (void)
{
	Application *app;
	BonoboUIHandlerMenuItem *menu_list;

	app = g_new0 (Application, 1);
	app->app = gnome_app_new ("test-container",
				  "Sample Container Application");
	app->container = BONOBO_CONTAINER (bonobo_container_new ());

	app->box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (app->box);
	gnome_app_set_contents (GNOME_APP (app->app), app->box);

	/*
	 * Create the menus.
	 */
	app->uih = bonobo_ui_handler_new ();

	bonobo_ui_handler_set_app (app->uih, GNOME_APP (app->app));
	bonobo_ui_handler_create_menubar (app->uih);

	menu_list = bonobo_ui_handler_menu_parse_uiinfo_list_with_data (container_main_menu, app);
	bonobo_ui_handler_menu_add_list (app->uih, "/", menu_list);
	bonobo_ui_handler_menu_free_list (menu_list);

	bonobo_ui_handler_create_toolbar (app->uih, "Common");
	bonobo_ui_handler_toolbar_new_item (app->uih,
					   "/Common/item 1",
					   "Container-added Item 1", "I am the container.  Hear me roar.",
					   0, BONOBO_UI_HANDLER_PIXMAP_NONE, NULL, 0, 0,
					   NULL, NULL);

	gtk_widget_show (app->app);

	return app;
}

int
main (int argc, char *argv [])
{
	Application *app;

	if (argc != 1)
		server_id = argv [1];
	
	CORBA_exception_init (&ev);
	
        gnome_init_with_popt_table ("MyShell", "1.0",
				    argc, argv,
				    oaf_popt_options, 0, NULL); 

	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init"));

	app = application_new ();
	
	bonobo_activate ();
	gtk_main ();

	CORBA_exception_free (&ev);
	
	return 0;
}
