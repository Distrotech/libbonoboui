/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-main.c: Bonobo Main
 *
 * Author:
 *    Miguel de Icaza  (miguel@gnu.org)
 *    Nat Friedman     (nat@nat.org)
 *    Peter Wainwright (prw@wainpr.demo.co.uk)
 *
 * Copyright 1999, 2001 Ximian, Inc.
 */
#include <config.h>

#include <bonobo/bonobo-i18n.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-ui-private.h>
#include <bonobo/bonobo-ui-main.h>
#include <bonobo/bonobo-main.h>

#include <libgnome/gnome-init.h>

#include <gtk/gtkmain.h>

#include <X11/Xlib.h>

static int (*gdk_x_error) (Display *, XErrorEvent *);

static int
bonobo_x_error_handler (Display *display, XErrorEvent *error)
{
	if (!error->error_code)
		return 0;

	/*
	 * If we got a BadDrawable or a BadWindow, we ignore it for
	 * now.  FIXME: We need to somehow distinguish real errors
	 * from X-server-induced errors.  Keeping a list of windows
	 * for which we will ignore BadDrawables would be a good idea.
	 */
	if (error->error_code == BadDrawable ||
	    error->error_code == BadWindow)
		return 0;

	/*
	 * Otherwise, let gdk deal.
	 */

	return gdk_x_error (display, error);
}

/**
 * bonobo_setup_x_error_handler:
 *
 * To do graphical embedding in the X window system, Bonobo
 * uses the classic foreign-window-reparenting trick.  The
 * GtkPlug/GtkSocket widgets are used for this purpose.  However,
 * serious robustness problems arise if the GtkSocket end of the
 * connection unexpectedly dies.  The X server sends out DestroyNotify
 * events for the descendants of the GtkPlug (i.e., your embedded
 * component's windows) in effectively random order.  Furthermore, if
 * you happened to be drawing on any of those windows when the
 * GtkSocket was destroyed (a common state of affairs), an X error
 * will kill your application.
 *
 * To solve this latter problem, Bonobo sets up its own X error
 * handler which ignores certain X errors that might have been
 * caused by such a scenario.  Other X errors get passed to gdk_x_error
 * normally.
 */
void
bonobo_setup_x_error_handler (void)
{
	static gboolean error_handler_setup = FALSE;

	if (error_handler_setup)
		return;

	error_handler_setup = TRUE;

	gdk_x_error = XSetErrorHandler (bonobo_x_error_handler);
}

static gboolean bonobo_ui_inited = FALSE;

gboolean
bonobo_ui_is_initialized (void)
{
	return bonobo_ui_inited;
}

gboolean
bonobo_ui_init (const gchar *app_name, const gchar *app_version,
		int *argc, char **argv)
{
	return bonobo_ui_init_full (app_name, app_version, argc, argv,
				    NULL, NULL, NULL, TRUE);
}

gboolean
bonobo_ui_init_full (const gchar *app_name, const gchar *app_version,
		     int *argc, char **argv, CORBA_ORB orb,
		     PortableServer_POA poa, PortableServer_POAManager manager,
		     gboolean full_init)
{
	if (bonobo_ui_inited)
		return TRUE;
	else
		bonobo_ui_inited = TRUE;

	if (!bonobo_init (argc, argv))
		return FALSE;

	g_set_prgname (app_name);

	if (full_init) {
		/* Initialize all our dependencies. */
		gnome_program_init (
			app_name, app_version,
			libgnome_module_info_get (),
			*argc, argv, NULL);
	}

	gtk_init (argc, &argv);

	bonobo_setup_x_error_handler ();

	return TRUE;
}

void
bonobo_ui_main (void)
{
	bonobo_activate ();

	gtk_main ();
}

int
bonobo_ui_debug_shutdown (void)
{
	if (bonobo_ui_preferences_shutdown ())
		return 1;
	
	return bonobo_debug_shutdown ();
}
