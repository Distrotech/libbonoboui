/*
 * test-libfilesel.c: a small test program for libgnomefilesel
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#include <config.h>

#include <bonobo/bonobo-file-selector-util.h>
#include <bonobo/bonobo-ui-main.h>

#include <gtk/gtkmain.h>

#include <libgnome/gnome-i18n.h>

static gint
get_files (gpointer data)
{
	char *s, **strv;
	int i;

	s = bonobo_file_selector_open (NULL, FALSE, NULL, NULL, "/etc");
	g_print ("open test:\n\t%s\n", s);
	g_free (s);

	s = bonobo_file_selector_save (NULL, FALSE, NULL, NULL, "/tmp", NULL);
	g_print ("save test:\n\t%s\n", s);
	g_free (s);

	strv = bonobo_file_selector_open_multi (NULL, TRUE, NULL, NULL, NULL);
	g_print ("open multi test:\n");
	if (strv) {
		for (i = 0; strv[i]; i++)
			g_print ("\t%s\n", strv[i]);

		g_strfreev (strv);
	}

	gtk_main_quit ();

	return FALSE;
}

int
main (int argc, char *argv[])
{

	free (malloc (8));

	textdomain (PACKAGE);

	if (!bonobo_ui_init ("test-filesel", VERSION, &argc, argv))
		g_error (_("Cannot bonobo_ui_init ()"));

	bonobo_activate ();

	g_idle_add (get_files, NULL);

	bonobo_main ();

	return 0;
}