/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Desktop Window Control implementation.
 * Enables applications to export their geometry control through CORBA.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtkplug.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-desktop-window.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_desktop_window_parent_class;

/* The entry point vectors for the server we provide */
POA_GNOME_Desktop_Window__vepv gnome_desktop_window_vepv;

GNOME_Desktop_Window
gnome_desktop_window_corba_object_create (GnomeObject *object)
{
	POA_GNOME_Desktop_Window *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_Desktop_Window *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_desktop_window_vepv;

	CORBA_exception_init (&ev);
	POA_GNOME_Desktop_Window__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (GNOME_Desktop_Window) gnome_object_activate_servant (object, servant);
}

/**
 * gnome_desktop_window_construct:
 * @desktop_win: The GnomeDesktopWindow object to be initialized.
 * @corba_desktop_win: The CORBA GNOME_Desktop_Window CORBA objeect.
 * @toplevel: Window we will have control over.
 *
 * Returns: the intialized GnomeDesktopWindow object.
 */
GnomeDesktopWindow *
gnome_desktop_window_construct (GnomeDesktopWindow *desk_win,
				GNOME_Desktop_Window corba_desktop_window,
				GtkWindow *toplevel)
{
	g_return_val_if_fail (desk_win != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_DESKTOP_WINDOW (desk_win), NULL);
	g_return_val_if_fail (corba_desktop_window != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (toplevel != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), NULL);
	
	gnome_object_construct (GNOME_OBJECT (desk_win), corba_desktop_window);

	desk_win->window = toplevel;
	return desk_win;
}

/**
 * gnome_desktop_window_new:
 * @toplevel: The toplevel Gtk window to control
 * container process.
 *
 */
GnomeDesktopWindow *
gnome_desktop_window_new (GtkWindow *toplevel)
{
	GnomeDesktopWindow *desktop_window;
	GNOME_Desktop_Window corba_desktop_window;
	
	g_return_val_if_fail (toplevel != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), NULL);

	desktop_window = gtk_type_new (gnome_desktop_window_get_type ());

	corba_desktop_window = gnome_desktop_window_corba_object_create (GNOME_OBJECT (desktop_window));
	if (corba_desktop_window == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (desktop_window));
		return NULL;
	}
	
	return gnome_desktop_window_construct (desktop_window, corba_desktop_window, toplevel);
}

static CORBA_char *
impl_desktop_window_get_title (PortableServer_Servant servant, CORBA_Environment * ev)
{
	GnomeDesktopWindow *desk_win = GNOME_DESKTOP_WINDOW (gnome_object_from_servant (servant));

	return CORBA_string_dup (desk_win->window->title);
}

static void
impl_desktop_window_set_title (PortableServer_Servant servant,
			       const CORBA_char *value,
			       CORBA_Environment * ev)
{
	GnomeDesktopWindow *desk_win = GNOME_DESKTOP_WINDOW (gnome_object_from_servant (servant));

	gtk_window_set_title (desk_win->window, value);
}

static GNOME_Desktop_Window_Geometry
impl_desktop_window_get_geometry (PortableServer_Servant servant,
				  CORBA_Environment * ev)
{
	GnomeDesktopWindow *desk_win = GNOME_DESKTOP_WINDOW (gnome_object_from_servant (servant));
	GNOME_Desktop_Window_Geometry geo;
	gint x, y;
	
	gdk_window_get_origin (GTK_WIDGET (desk_win->window)->window, &x, &y);
	geo.x = x;
	geo.y = y;
	geo.width = GTK_WIDGET (desk_win->window)->allocation.width;
	geo.height = GTK_WIDGET (desk_win->window)->allocation.height;

	return geo;
}

static void
impl_desktop_window_set_geometry (PortableServer_Servant               servant,
				  const GNOME_Desktop_Window_Geometry *geo,
				  CORBA_Environment                   *ev)
{
	GnomeDesktopWindow *desk_win = GNOME_DESKTOP_WINDOW (gnome_object_from_servant (servant));

	gtk_widget_set_uposition (GTK_WIDGET (desk_win->window), geo->x, geo->y);
	gtk_widget_set_usize (GTK_WIDGET (desk_win->window), geo->width, geo->height);
}

static CORBA_unsigned_long
impl_desktop_window_get_window_id (PortableServer_Servant servant, CORBA_Environment * ev)
{
	GnomeDesktopWindow *desk_win = GNOME_DESKTOP_WINDOW (gnome_object_from_servant (servant));

	return GDK_WINDOW_XWINDOW (GTK_WIDGET (desk_win->window)->window);
}

/**
 * gnome_desktop_window_get_epv:
 */
POA_GNOME_Desktop_Window__epv *
gnome_desktop_window_get_epv (void)
{
	POA_GNOME_Desktop_Window__epv *epv;

	epv = g_new0 (POA_GNOME_Desktop_Window__epv, 1);

	epv->_get_title = impl_desktop_window_get_title;
	epv->_set_title = impl_desktop_window_set_title;
	epv->get_geometry = impl_desktop_window_get_geometry;
	epv->set_geometry = impl_desktop_window_set_geometry;
	epv->get_window_id = impl_desktop_window_get_window_id;

	return epv;
}

static void
init_desktop_window_corba_class (void)
{
	/* Setup the vector of epvs */
	gnome_desktop_window_vepv.GNOME_Unknown_epv = gnome_object_get_epv ();
	gnome_desktop_window_vepv.GNOME_Desktop_Window_epv = gnome_desktop_window_get_epv ();
}

static void
gnome_desktop_window_class_init (GnomeDesktopWindowClass *klass)
{
	gnome_desktop_window_parent_class = gtk_type_class (gnome_object_get_type ());
	init_desktop_window_corba_class ();
}

static void
gnome_desktop_window_init (GnomeDesktopWindow *desktop_window)
{
}

/**
 * gnome_desktop_window_get_type:
 *
 * Returns: The GtkType corresponding to the GnomeDesktopWindow class.
 */
GtkType
gnome_desktop_window_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/Desktop_Window:1.0",
			sizeof (GnomeDesktopWindow),
			sizeof (GnomeDesktopWindowClass),
			(GtkClassInitFunc) gnome_desktop_window_class_init,
			(GtkObjectInitFunc) gnome_desktop_window_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_desktop_window_control:
 * @object: Object to be aggregated.
 * @win: Window to be controled.
 *
 * Attaches a GNOME::Desktop::Window corba handler to a GNOME
 * object controlling the window @win.  
 */
void
gnome_desktop_window_control (GnomeObject *object, GtkWindow *win)
{
	GnomeObject *win_control_object;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (win != NULL);
	g_return_if_fail (GNOME_IS_OBJECT (object));
	g_return_if_fail (GTK_IS_WINDOW (win));

	win_control_object = GNOME_OBJECT (gnome_desktop_window_new (win));
	gnome_object_add_interface (object, win_control_object);
}
