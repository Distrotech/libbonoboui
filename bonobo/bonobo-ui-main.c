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

#undef GTK_DISABLE_DEPRECATED

#include <config.h>

#include <glib/gi18n-lib.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-ui-private.h>
#include <bonobo/bonobo-ui-main.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-a11y.h>

#include <libgnome/gnome-init.h>

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <string.h>
#include <libxml/xmlIO.h>
#include <libxml/uri.h>
#include <glib/gstdio.h>
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

static int (*gdk_x_error_func) (Display *, XErrorEvent *);

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

	return gdk_x_error_func (display, error);
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

	gdk_x_error_func = XSetErrorHandler (bonobo_x_error_handler);
}

#endif

static gboolean bonobo_ui_inited = FALSE;

gboolean
bonobo_ui_is_initialized (void)
{
	return bonobo_ui_inited;
}

#ifdef G_OS_WIN32

static void *
utf8_file_open_real (const char *filename)
{
    const char *path = NULL;
    FILE *fd;

    if (filename == NULL)
        return (NULL);

    if (!strcmp (filename, "-")) {
	fd = stdin;
	return ((void *) fd);
    }

    if (!xmlStrncasecmp (filename, "file://localhost/", 17))
	    path = &filename[17];
    else if (!xmlStrncasecmp (filename, "file:///", 8)) {
	    path = &filename[8];
    } else 
	    path = filename;

    if (path == NULL)
	    return (NULL);
    if (!g_file_test (path, G_FILE_TEST_IS_REGULAR))
	    return (NULL);

    fd = g_fopen (path, "rb");
    if (fd == NULL)
	    return (NULL);

    return ((void *) fd);
}

static void *
utf8_file_open (const char *filename)
{
	char *unescaped;
	void *retval;

	unescaped = xmlURIUnescapeString (filename, 0, NULL);
	if (unescaped != NULL) {
		retval = utf8_file_open_real (unescaped);
		xmlFree (unescaped);
	} else {
		retval = utf8_file_open_real (filename);
	}
	return retval;
	
}

#endif

static void
do_low_level_init (void)
{
#ifdef GDK_WINDOWING_X11
	CORBA_Context context;
	CORBA_Environment ev;
	GdkDisplay *display;
	Display *xdisplay;
#endif

	if (bonobo_ui_inited)
		return;

	bonobo_ui_inited = TRUE;

	gtk_set_locale ();
	bindtextdomain (GETTEXT_PACKAGE, BONOBO_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

#ifdef G_OS_WIN32
	/* This is a suitable place to register our own libxml2 file
	 * open callback that takes UTF-8 filenames.
	 */
	xmlRegisterInputCallbacks (xmlFileMatch, utf8_file_open,
				   xmlFileRead, xmlFileClose);
#endif

#ifdef GDK_WINDOWING_X11
	bonobo_setup_x_error_handler ();

	display = gdk_display_get_default ();
	xdisplay = gdk_x11_display_get_xdisplay (display);
		
	CORBA_exception_init (&ev);

	/* FIXME: nasty contractual bonobo-activation issues here */
	context = bonobo_activation_context_get ();
	CORBA_Context_set_one_value (
		context, "display",
		DisplayString (xdisplay),
		&ev);

	CORBA_exception_free (&ev);
#endif
}

/* compat */
gboolean
bonobo_ui_init (const gchar *app_name, const gchar *app_version,
		int *argc, char **argv)
{
	return bonobo_ui_init_full (app_name, app_version, argc, argv,
				    NULL, NULL, NULL, TRUE);
}

/* compat */
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

		gtk_set_locale ();
		bindtextdomain (GETTEXT_PACKAGE, BONOBO_LOCALEDIR);
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

		gnome_program_init (
			app_name, app_version,
			libgnome_module_info_get (),
			*argc, argv, NULL);
	}

	gtk_init (argc, &argv);

	do_low_level_init ();

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

static void
libbonoboui_post_args_parse (GnomeProgram    *program,
			     GnomeModuleInfo *mod_info)
{
	do_low_level_init ();
}

const GnomeModuleInfo *
libbonobo_ui_module_info_get (void)
{
	static GnomeModuleInfo module_info = {
		"libbonoboui", VERSION,
		N_("Bonobo GUI support"),
		NULL, NULL,
		NULL, libbonoboui_post_args_parse,
		NULL, NULL, NULL, NULL
	};

	if (module_info.requirements == NULL) {
		static GnomeModuleRequirement req[6];

		req[0].required_version = "1.3.7";
		req[0].module_info = bonobo_ui_gtk_module_info_get ();

		req[1].required_version = "1.102.0";
		req[1].module_info = LIBGNOME_MODULE;

		req[2].required_version = "1.101.2";
		req[2].module_info = GNOME_BONOBO_MODULE;

		req[5].required_version = NULL;
		req[5].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}
