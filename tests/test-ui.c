/*
 * app.c: A test application to hammer the Bonobo UI api.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include "config.h"
#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-win.h>

poptContext ctx;

static int
cb_do_quit (GtkWindow *window, gpointer dummy)
{
	gtk_main_quit ();
	return 1;
}

static int
cb_do_dump (GtkWindow *window, BonoboWin *app)
{
	bonobo_win_dump (app, "on User input");
	return 1;
}

static void
toggled_cb (BonoboUIComponent           *component,
	    const char                  *path,
	    Bonobo_UIComponent_EventType type,
	    const char                  *state,
	    gpointer                     user_data)
{
	fprintf (stderr, "toggled to '%s' type '%d' path '%s'\n",
		 state, type, path);
}

int
main (int argc, char **argv)
{
	BonoboWin *app;
	CORBA_ORB  orb;
	BonoboUIComponent *componenta;
	BonoboUIComponent *componentb;
	BonoboUIComponent *componentc;
	BonoboUIContainer *container;
	CORBA_Environment  ev;
	Bonobo_UIContainer corba_container;

	char simplea [] =
		"<submenu name=\"File\" label=\"_File\">\n"
		"	<menuitem name=\"open\" label=\"_Open\" pixtype=\"stock\" pixname=\"Menu_Open\" descr=\"Wibble\"/>\n"
		"</submenu>\n";
	char simpleb [] =
		"<submenu name=\"File\" label=\"_FileB\">\n"
		"	<menuitem name=\"open\" label=\"_OpenB\" pixtype=\"stock\" pixname=\"Menu_Open\" descr=\"Open you fool\"/>\n"
		"       <menuitem/>\n"
		"       <menuitem name=\"toggle\" type=\"toggle\" id=\"MyFoo\" label=\"_ToggleMe\" accel=\"&lt;Control&gt;t\"/>\n"
		"       <placeholder delimit=\"both\"/>\n"
		"	<menuitem name=\"close\" noplace=\"1\" verb=\"Close\" label=\"_CloseB\" "
		"        pixtype=\"stock\" pixname=\"Menu_Close\" accel=\"&lt;Control&gt;q\"/>\n"
		"</submenu>\n";
	char simplec [] =
		"<submenu name=\"File\" label=\"_FileC\">\n"
		"    <placeholder>\n"
		"	<menuitem name=\"fooa\" label=\"_FooA\" type=\"radio\" group=\"foogroup\" descr=\"Radio1\"/>\n"
		"	<menuitem name=\"foob\" label=\"_FooB\" type=\"radio\" group=\"foogroup\"/>\n"
		"	<menuitem name=\"wibble\" label=\"_Baa\" pixtype=\"stock\" pixname=\"Menu_Open\" sensitive=\"0\"/>\n"
		"    </placeholder>\n"
		"</submenu>\n";
	char simpled [] =
		"<menuitem name=\"save\" label=\"_SaveD\" pixtype=\"stock\" pixname=\"Menu_Save\"/>\n";
	char toola [] =
		"<dockitem name=\"toolbar\">\n"
		"	<toolitem type=\"toggle\" name=\"foo2\" id=\"MyFoo\"pixtype=\"stock\" pixname=\"Save\" label=\"TogSave\" descr=\"My tooltip\"/>\n"
		"	<toolitem type=\"separator\" name=\"foo3\" pixtype=\"stock\" pixname=\"Save\" label=\"Separator\"/>\n"
		"	<toolitem type=\"std\" name=\"baa\" pixtype=\"stock\" pixname=\"Open\" label=\"baa\" descr=\"My 2nd tooltip\" verb=\"testme\"/>\n"
		"</dockitem>";
	char toolb [] =
		"<dockitem name=\"toolbar\" look=\"icon\" relief=\"none\">\n"
		"	<toolitem name=\"foo1\" type=\"std\" label=\"Insensitive\" sensitive=\"0\"/>\n"
		"	<toolitem type=\"toggle\" name=\"foo5\" id=\"MyFoo\" pixtype=\"stock\" pixname=\"Close\" label=\"TogSame\" descr=\"My tooltip\"/>\n"
		"</dockitem>";
	char statusa [] =
		"<item name=\"main\">Kippers</item>\n";
	char statusb [] =
		"<item name=\"main\">Nothing</item>\n";
	xmlNode *accel, *file;

	free (malloc (8));

	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	bonobo_activate ();

	app = BONOBO_WIN (bonobo_win_new ("App", "My Test Application"));
	container = bonobo_ui_container_new ();
	bonobo_ui_container_set_app (container, app);

	corba_container = bonobo_object_corba_objref (BONOBO_OBJECT (container));

	{
		GtkWidget *box = gtk_vbox_new (FALSE, 0);
		GtkWidget *button;

		button = gtk_button_new_with_label ("Xml merge / demerge");
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) cb_do_quit, NULL);
		gtk_widget_show (GTK_WIDGET (button));
		gtk_box_pack_start_defaults (GTK_BOX (box), button);

		button = gtk_button_new_with_label ("Dump Xml tree");
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) cb_do_dump, app);
		gtk_widget_show (GTK_WIDGET (button));
		gtk_box_pack_start_defaults (GTK_BOX (box), button);

		gtk_widget_show (GTK_WIDGET (box));
		bonobo_win_set_contents (app, box);
	}

	componenta = bonobo_ui_component_new ("A");
	componentb = bonobo_ui_component_new ("B");
	componentc = bonobo_ui_component_new ("C");

	CORBA_exception_init (&ev);

	bonobo_ui_component_set (componentb, corba_container, "/status", statusa, &ev);

	bonobo_ui_component_set (componenta, corba_container, "/menu", simplea, &ev);

	{
		GtkWidget *widget = gtk_button_new_with_label ("My Label");
		BonoboControl *control = bonobo_control_new (widget);
		
		gtk_widget_show (widget);
		bonobo_ui_container_object_set (
			corba_container,
			"/menu/submenu/#File/control/#MyControl",
			bonobo_object_corba_objref (BONOBO_OBJECT (control)),
			NULL);
	}

	{
		GtkWidget *widget = gtk_entry_new ();
		BonoboControl *control = bonobo_control_new (widget);
		
		gtk_entry_set_text (GTK_ENTRY (widget), "Example text");
		gtk_widget_show (widget);
		bonobo_ui_container_object_set (
			corba_container,
			"/dockitem/#toolbar/control/#AControl",
			bonobo_object_corba_objref (BONOBO_OBJECT (control)),
			NULL);
	}

	bonobo_ui_component_set (componentb, corba_container, "/",     toola, &ev);

	bonobo_ui_component_add_listener (componentb, "MyFoo", toggled_cb, NULL);

	gtk_widget_show (GTK_WIDGET (app));

	gtk_main ();

	accel = bonobo_ui_util_build_accel (GDK_A, GDK_CONTROL_MASK, "KeyWibbleVerb");
	bonobo_ui_component_set_tree (componenta, corba_container, "/keybindings", accel, &ev);

	bonobo_ui_component_set (componentb, corba_container, "/menu", simpleb, &ev);
	bonobo_ui_component_set (componenta, corba_container, "/",     toolb, &ev);
	bonobo_ui_component_set (componentb, corba_container, "/status", statusb, &ev);

	{
		GtkWidget *widget = gtk_button_new_with_label ("A progress bar");
		BonoboControl *control = bonobo_control_new (widget);

/*		gtk_progress_bar_update (GTK_PROGRESS_BAR (widget), 0.5);*/
		gtk_widget_show (widget);
		bonobo_ui_container_object_set (
			corba_container,
			"/status/control/#Progress",
			bonobo_object_corba_objref (BONOBO_OBJECT (control)),
			NULL);
	}
	gtk_main ();

	bonobo_ui_component_set (componentc, corba_container, "/commands",
				 "<cmd name=\"MyFoo\" sensitive=\"0\"/>", &ev);
	bonobo_ui_component_set (componentc, corba_container, "/menu", simplec, &ev);
	
	bonobo_ui_component_set (componentc, corba_container, "/menu/submenu/#File", simpled, &ev);

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 2 ---\n\n\n");
	bonobo_ui_component_rm (componentb, corba_container, "/", &ev);

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 3 ---\n\n\n");
	bonobo_ui_component_rm (componentc, corba_container, "/", &ev);

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 1 ---\n\n\n");
	bonobo_ui_component_rm (componenta, corba_container, "/", &ev);

	gtk_main ();

	if (g_file_exists ("ui.xml")) {
		fprintf (stderr, "\n\n--- Add ui.xml ---\n\n\n");
		file = bonobo_ui_util_new_ui (componentc, "ui.xml", "gnomecal");
		bonobo_ui_component_set_tree (componentc, corba_container,
					      "/", file, &ev);
		gtk_main ();

	} else
		g_warning ("Can't find ui.xml");

	bonobo_object_unref (BONOBO_OBJECT (componenta));
	bonobo_object_unref (BONOBO_OBJECT (componentb));
	bonobo_object_unref (BONOBO_OBJECT (componentc));

	bonobo_object_unref (BONOBO_OBJECT (container));
	gtk_widget_destroy (GTK_WIDGET (app));

	CORBA_exception_free (&ev);

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
