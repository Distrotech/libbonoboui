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

Bonobo_UIContainer corba_container;

static void
cb_do_quit (GtkWindow *window, gpointer dummy)
{
	gtk_main_quit ();
}

static void
cb_do_dump (GtkWindow *window, BonoboWin *win)
{
	bonobo_win_dump (win, "on User input");
}

static void
cb_do_popup (GtkWindow *window, BonoboWin *win)
{
	GtkWidget *menu;
	GtkWidget *menuitem;

	menu = gtk_menu_new ();
	menuitem = gtk_menu_item_new_with_label ("Error if you see this");
	gtk_widget_show (menuitem);
	gtk_menu_append (GTK_MENU (menu), menuitem);

	bonobo_win_add_popup (win, GTK_MENU (menu), "/popups/MyStuff");

	gtk_widget_show (GTK_WIDGET (menu));
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 0);
}

static void
cb_set_state (GtkEntry *state_entry, GtkEntry *path_entry)
{
	char *path, *state, *txt, *str;

	path = gtk_entry_get_text (path_entry);
	state = gtk_entry_get_text (state_entry);

	g_warning ("Set state on '%s' to '%s'", path, state);

	bonobo_ui_container_set_prop (
		corba_container, path, "state", state, NULL);

	txt = bonobo_ui_container_get_prop (
		corba_container, path, "state", NULL);

	g_warning ("Re-fetched state was '%s'", txt);

	str = g_strdup_printf ("The state is now '%s'", txt);
	bonobo_ui_container_set_status (corba_container, str, NULL);
	g_free (str);

	g_free (txt);
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

static void
disconnect_progress (GtkObject *progress, gpointer dummy)
{
	gtk_timeout_remove (GPOINTER_TO_UINT (dummy));
}

static gboolean
update_progress (GtkProgress *progress)
{
	float pos = gtk_progress_get_current_percentage (progress);

	if (pos < 0.95)
		gtk_progress_set_percentage (progress, pos + 0.05);
	else
		gtk_progress_set_percentage (progress, 0);

	return TRUE;
}

int
main (int argc, char **argv)
{
	BonoboWin *win;
	CORBA_ORB  orb;
	BonoboUIComponent *componenta;
	BonoboUIComponent *componentb;
	BonoboUIComponent *componentc;
	BonoboUIContainer *container;
	CORBA_Environment  ev;

	char simplea [] =
		"<menu>\n"
		"	<submenu name=\"File\" label=\"_File\">\n"
		"		<menuitem name=\"open\" label=\"_Open\" pixtype=\"stock\" pixname=\"Menu_Open\" descr=\"Wibble\"/>\n"
		"		<control name=\"MyControl\"/>\n"
		"	</submenu>\n"
		"</menu>";
	char simpleb [] =
		"<submenu name=\"File\" label=\"_FileB\">\n"
		"	<menuitem name=\"open\" label=\"_OpenB\" pixtype=\"stock\" pixname=\"Menu_Open\" descr=\"Open you fool\"/>\n"
		"       <menuitem/>\n"
		"       <menuitem name=\"toggle\" type=\"toggle\" id=\"MyFoo\" label=\"_ToggleMe\" accel=\"*Control*t\"/>\n"
		"       <placeholder name=\"Nice\" delimit=\"both\"/>\n"
		"	<menuitem name=\"close\" noplace=\"1\" verb=\"Close\" label=\"_CloseB\" "
		"        pixtype=\"stock\" pixname=\"Menu_Close\" accel=\"*Control*q\"/>\n"
		"</submenu>\n";
	char simplec [] =
		"<submenu name=\"File\" label=\"_FileC\">\n"
		"    <placeholder name=\"Nice\" delimit=\"both\">\n"
		"	<menuitem name=\"fooa\" label=\"_FooA\" type=\"radio\" group=\"foogroup\" descr=\"Radio1\"/>\n"
		"	<menuitem name=\"foob\" label=\"_FooB\" type=\"radio\" group=\"foogroup\"/>\n"
		"	<menuitem name=\"wibble\" label=\"_Baa\" pixtype=\"stock\" pixname=\"Menu_Open\" sensitive=\"0\"/>\n"
		"    </placeholder>\n"
		"</submenu>\n";
	char simpled [] =
		"<menuitem name=\"save\" label=\"_SaveD\" pixtype=\"stock\" pixname=\"Menu_Save\"/>\n";
	char simplee [] =
		"<menuitem name=\"fish\" label=\"_Inplace\" pixtype=\"stock\" pixname=\"Menu_Save\"/>\n";
	char toola [] =
		"<dockitem name=\"toolbar\" homogeneous=\"0\" look=\"both\">\n"
		"	<toolitem type=\"toggle\" name=\"foo2\" id=\"MyFoo\"pixtype=\"stock\" pixname=\"Save\" label=\"TogSave\" descr=\"My tooltip\"/>\n"
		"	<toolitem type=\"separator\" name=\"foo3\" pixtype=\"stock\" pixname=\"Save\" label=\"Separator\"/>\n"
		"	<toolitem type=\"std\" name=\"baa\" pixtype=\"stock\" pixname=\"Open\" label=\"baa\" descr=\"My 2nd tooltip\" verb=\"testme\"/>\n"
		"	<control name=\"AControl\"/>\n"
		"</dockitem>";
	char toolb [] =
		"<dockitem name=\"toolbar\" look=\"icon\" relief=\"none\">\n"
		"	<toolitem name=\"foo1\" type=\"std\" label=\"Insensitive\" sensitive=\"0\"/>\n"
		"	<toolitem type=\"toggle\" name=\"foo5\" id=\"MyFoo\" pixtype=\"stock\" pixname=\"Close\" label=\"TogSame\" descr=\"My tooltip\"/>\n"
		"</dockitem>";
	char statusa [] =
		"<item name=\"main\">Kippers</item>\n";
	char statusb [] =
		"<status>\n"
		"	<item name=\"main\">Nothing</item>\n"
		"	<control name=\"Progress\"/>\n"
		"</status>";
	BonoboUINode *accel;

	free (malloc (8));

	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	bonobo_activate ();

	win = BONOBO_WIN (bonobo_win_new ("Win", "My Test Application"));
	container = bonobo_ui_container_new ();
	bonobo_ui_container_set_app (container, win);

	corba_container = bonobo_object_corba_objref (BONOBO_OBJECT (container));

	{
		GtkWidget *box = gtk_vbox_new (FALSE, 0);
		GtkWidget *button;
		GtkWidget *path_entry, *state_entry;

		button = gtk_button_new_with_label ("Xml merge / demerge");
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) cb_do_quit, NULL);
		gtk_widget_show (GTK_WIDGET (button));
		gtk_box_pack_start_defaults (GTK_BOX (box), button);

		button = gtk_button_new_with_label ("Dump Xml tree");
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) cb_do_dump, win);
		gtk_widget_show (GTK_WIDGET (button));
		gtk_box_pack_start_defaults (GTK_BOX (box), button);

		button = gtk_button_new_with_label ("Popup");
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
				    (GtkSignalFunc) cb_do_popup, win);
		gtk_widget_show (GTK_WIDGET (button));
		gtk_box_pack_start_defaults (GTK_BOX (box), button);

		path_entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (path_entry), "/menu/File/toggle");
		gtk_widget_show (GTK_WIDGET (path_entry));
		gtk_box_pack_start_defaults (GTK_BOX (box), path_entry);

		state_entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (state_entry), "1");
		gtk_signal_connect (GTK_OBJECT (state_entry), "changed",
				    (GtkSignalFunc) cb_set_state, path_entry);
		gtk_widget_show (GTK_WIDGET (state_entry));
		gtk_box_pack_start_defaults (GTK_BOX (box), state_entry);

		gtk_widget_show (GTK_WIDGET (box));
		bonobo_win_set_contents (win, box);
	}

	componenta = bonobo_ui_component_new ("A");
	componentb = bonobo_ui_component_new ("B");
	componentc = bonobo_ui_component_new ("C");

	CORBA_exception_init (&ev);

	if (g_file_exists ("ui.xml")) {
		fprintf (stderr, "\n\n--- Add ui.xml ---\n\n\n");
		bonobo_ui_util_set_ui (componenta, corba_container,
				       NULL, "ui.xml", "gnomecal");
		bonobo_ui_container_thaw (corba_container, NULL);

		gtk_widget_show (GTK_WIDGET (win));

		gtk_main ();
	} else
		g_warning ("Can't find ui.xml");

	bonobo_ui_container_freeze (corba_container, NULL);

	fprintf (stderr, "\n\n--- Remove A ---\n\n\n");
	bonobo_ui_component_rm (componenta, corba_container, "/", &ev);

	bonobo_ui_component_set (componentb, corba_container, "/status", statusa, &ev);

	bonobo_ui_component_set (componenta, corba_container, "/", simplea, &ev);

	bonobo_ui_component_set (componentb, corba_container, "/",
				 "<popups> <popup name=\"MyStuff\"/> </popups>", &ev);
	bonobo_ui_component_set (componenta, corba_container, "/popups/MyStuff", simpleb, &ev);

	bonobo_ui_component_set (componentb, corba_container, "/",   toola, &ev);

	{
		GtkWidget *widget = gtk_button_new_with_label ("My Label");
		BonoboControl *control = bonobo_control_new (widget);
		
		gtk_widget_show (widget);
		bonobo_ui_container_object_set (
			corba_container,
			"/menu/File/MyControl",
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
			"/toolbar/AControl",
			bonobo_object_corba_objref (BONOBO_OBJECT (control)),
			NULL);
	}

	bonobo_ui_component_add_listener (componentb, "MyFoo", toggled_cb, NULL);

	bonobo_ui_component_set (componentb, corba_container, "/",     statusb, &ev);

	/* Duplicate set */
	bonobo_ui_component_set (componenta, corba_container, "/", simplea, &ev);

	bonobo_ui_container_thaw (corba_container, NULL);

	gtk_main ();

	bonobo_ui_container_freeze (corba_container, NULL);

	accel = bonobo_ui_util_build_accel (GDK_A, GDK_CONTROL_MASK, "KeyWibbleVerb");
	bonobo_ui_component_set_tree (componenta, corba_container, "/keybindings", accel, &ev);
	bonobo_ui_node_free (accel);

	bonobo_ui_component_set (componentb, corba_container, "/menu", simpleb, &ev);
	bonobo_ui_component_set (componenta, corba_container, "/",     toolb, &ev);
	bonobo_ui_component_set (componenta, corba_container, "/menu/File/Nice", simplee, &ev);

	{
		GtkWidget *widget = gtk_progress_bar_new ();
		BonoboControl *control = bonobo_control_new (widget);
		guint id;

		gtk_progress_bar_update (GTK_PROGRESS_BAR (widget), 0.5);
		gtk_widget_show (widget);
		bonobo_ui_container_object_set (
			corba_container,
			"/status/Progress",
			bonobo_object_corba_objref (BONOBO_OBJECT (control)),
			NULL);

		id = gtk_timeout_add (100, (GSourceFunc) update_progress, widget);
		gtk_signal_connect (GTK_OBJECT (widget), "destroy",
				    disconnect_progress, GUINT_TO_POINTER (id));
	}

	bonobo_ui_container_thaw (corba_container, NULL);
	gtk_main ();
	bonobo_ui_container_freeze (corba_container, NULL);

	bonobo_ui_component_set (componentc, corba_container, "/commands",
				 "<cmd name=\"MyFoo\" sensitive=\"0\"/>", &ev);
	bonobo_ui_component_set (componentc, corba_container, "/menu", simplec, &ev);
	
	bonobo_ui_component_set (componentc, corba_container, "/menu/File", simpled, &ev);

	bonobo_ui_container_thaw (corba_container, NULL);
	gtk_main ();
	bonobo_ui_container_freeze (corba_container, NULL);

	fprintf (stderr, "\n\n--- Remove 2 ---\n\n\n");
	bonobo_ui_component_rm (componentb, corba_container, "/", &ev);

	bonobo_ui_container_thaw (corba_container, NULL);
	gtk_main ();
	bonobo_ui_container_freeze (corba_container, NULL);

	fprintf (stderr, "\n\n--- Remove 3 ---\n\n\n");
	bonobo_ui_component_rm (componentc, corba_container, "/", &ev);

	bonobo_ui_container_thaw (corba_container, NULL);
	gtk_main ();
	bonobo_ui_container_freeze (corba_container, NULL);

	fprintf (stderr, "\n\n--- Remove 1 ---\n\n\n");
	bonobo_ui_component_rm (componenta, corba_container, "/", &ev);

	bonobo_ui_container_thaw (corba_container, NULL);
	gtk_main ();

	bonobo_object_unref (BONOBO_OBJECT (componenta));
	bonobo_object_unref (BONOBO_OBJECT (componentb));
	bonobo_object_unref (BONOBO_OBJECT (componentc));

	bonobo_object_unref (BONOBO_OBJECT (container));
	gtk_widget_destroy (GTK_WIDGET (win));

	CORBA_exception_free (&ev);

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
