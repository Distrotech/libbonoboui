#include "config.h"
#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

#include "bonobo-ui-xml.h"
#include "bonobo-ui-util.h"
#include "bonobo-app.h"

poptContext ctx;

static int
cb_do_quit (GtkWindow *window, gpointer dummy)
{
	gtk_main_quit ();
	return 1;
}

static int
cb_do_dump (GtkWindow *window, BonoboApp *app)
{
	bonobo_app_dump (app, "on User input");
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
	BonoboApp *app;
	CORBA_ORB  orb;
	BonoboUIComponent *componenta;
	BonoboUIComponent *componentb;
	BonoboUIComponent *componentc;
	CORBA_Environment  ev;
	Bonobo_UIContainer corba_app;

	char simplea [] =
		"<submenu name=\"file\" label=\"_File\">\n"
		"	<menuitem name=\"open\" label=\"_Open\" pixtype=\"stock\" pixname=\"Menu_Open\"/>\n"
		"</submenu>\n";
	char simpleb [] =
		"<submenu name=\"file\" label=\"_FileB\">\n"
		"	<menuitem name=\"open\" label=\"_OpenB\" pixtype=\"stock\" pixname=\"Menu_Open\"/>\n"
		"       <menuitem name=\"toggle\" type=\"toggle\" id=\"MyFoo\" label=\"_ToggleMe\"/>\n"
		"       <placeholder delimit=\"both\"/>\n"
		"	<menuitem name=\"close\" noplace=\"1\" verb=\"Close\" label=\"_CloseB\" pixtype=\"stock\" pixname=\"Menu_Close\"/>\n"
		"</submenu>\n";
	char simplec [] =
		"<submenu name=\"file\" label=\"_FileC\">\n"
		"    <placeholder>\n"
		"	<menuitem name=\"fooa\" label=\"_FooA\" type=\"radio\" group=\"foogroup\"/>\n"
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
		"	<toolitem type=\"std\" name=\"baa\" pixtype=\"stock\" pixname=\"Open\" label=\"baa\" descr=\"My 2nd tooltip\"/>\n"
		"</dockitem>";
	char toolb [] =
		"<dockitem name=\"toolbar\" look=\"icon\" relief=\"none\">\n"
		"	<toolitem name=\"foo1\" type=\"std\" label=\"Insensitive\" sensitive=\"0\"/>\n"
		"	<toolitem type=\"toggle\" name=\"foo5\" id=\"MyFoo\" pixtype=\"stock\" pixname=\"Close\" label=\"TogSame\" descr=\"My tooltip\"/>\n"
		"</dockitem>";
	char statusa [] =
		"<status>\n"
		"	<statusitem name=\"left\"/>\n"
		"</status>\n";
	xmlNode *help, *accel;

	free (malloc (8));

	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	bonobo_activate ();

	app = BONOBO_APP (bonobo_app_new ("App", "My Test Application"));
	corba_app = bonobo_object_corba_objref (BONOBO_OBJECT (app));

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
		bonobo_app_set_contents (app, box);
	}

	componenta = bonobo_ui_component_new ("A");
	componentb = bonobo_ui_component_new ("B");
	componentc = bonobo_ui_component_new ("C");

	CORBA_exception_init (&ev);

	bonobo_ui_component_set (componenta, corba_app, "/menu", simplea, &ev);

	bonobo_ui_component_set (componentb, corba_app, "/",     toola, &ev);

	bonobo_ui_component_add_listener (componentb, "MyFoo", toggled_cb, NULL);

	gtk_main ();

	help = bonobo_ui_util_build_help_menu (componenta, "gnomecal");
	bonobo_ui_component_set_tree (componenta, corba_app, "/menu", help, &ev);

	accel = bonobo_ui_util_build_accel (GDK_A, GDK_CONTROL_MASK, "KeyWibbleVerb");
	bonobo_ui_component_set_tree (componenta, corba_app, "/keybindings", accel, &ev);

	bonobo_ui_component_set (componentb, corba_app, "/menu", simpleb, &ev);
	bonobo_ui_component_set (componenta, corba_app, "/",     toolb, &ev);

	gtk_main ();

	bonobo_ui_component_set (componentc, corba_app, "/commands",
				 "<cmd name=\"MyFoo\" sensitive=\"0\"/>", &ev);
	bonobo_ui_component_set (componentc, corba_app, "/menu", simplec, &ev);
	
	bonobo_ui_component_set (componentc, corba_app, "/menu/submenu/#file", simpled, &ev);

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 2 ---\n\n\n");
	bonobo_ui_component_rm (componentb, corba_app, "/", &ev);

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 3 ---\n\n\n");
	bonobo_ui_component_rm (componentc, corba_app, "/", &ev);

	gtk_main ();

	bonobo_object_unref (BONOBO_OBJECT (componenta));
	bonobo_object_unref (BONOBO_OBJECT (componentb));
	bonobo_object_unref (BONOBO_OBJECT (componentc));

	bonobo_object_unref (BONOBO_OBJECT (app));

	CORBA_exception_free (&ev);

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
