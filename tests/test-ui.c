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

	char simplea [] =
		"<submenu name=\"file\" label=\"_File\">\n"
		"	<menuitem name=\"open\" label=\"_Open\" pixtype=\"stock\" pixname=\"Menu_Open\"/>\n"
		"</submenu>\n";
	char simpleb [] =
		"<submenu name=\"file\" label=\"_FileB\">\n"
		"	<menuitem name=\"open\" label=\"_OpenB\" pixtype=\"stock\" pixname=\"Menu_Open\"/>\n"
		"       <menuitem name=\"toggle\" type=\"toggle\" label=\"_ToggleMe\"/>\n"
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
		"</dockitem>";
	char statusa [] =
		"<status>\n"
		"	<statusitem name=\"left\"/>\n"
		"</status>\n";
	xmlNode *help;

	free (malloc (8));

	gnome_init_with_popt_table ("container", VERSION,
				    argc, argv, oaf_popt_options, 0, &ctx);

	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!\n"));

	bonobo_activate ();

	app = bonobo_app_new ("App", "My Test Application");

	{
		GtkWidget *button = gtk_button_new_with_label ("Xml merge / demerge");

		gtk_widget_show (GTK_WIDGET (button));
		bonobo_app_set_contents (app, button);
		
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
		(GtkSignalFunc)cb_do_quit, NULL);
	}

	componenta = bonobo_ui_component_new ();
	componentb = bonobo_ui_component_new ();
	componentc = bonobo_ui_component_new ();

	bonobo_app_xml_merge (app, "/menu", simplea,
			      bonobo_object_corba_objref (BONOBO_OBJECT (componenta)));
	bonobo_app_xml_merge (app, "/",     toola,  
			      bonobo_object_corba_objref (BONOBO_OBJECT (componentb)));

	bonobo_ui_component_add_listener (componentb, "MyFoo", toggled_cb, NULL);
	gtk_main ();

	help = bonobo_ui_build_help_menu (componenta, "gnomecal");
	bonobo_app_xml_merge_tree (app, "/menu", help,
				   bonobo_object_corba_objref (BONOBO_OBJECT (componenta)));

	bonobo_app_xml_merge (app, "/menu", simpleb,
			      bonobo_object_corba_objref (BONOBO_OBJECT (componentb)));
	bonobo_app_xml_merge (app, "/",     toolb,  
			      bonobo_object_corba_objref (BONOBO_OBJECT (componenta)));

	gtk_main ();
	bonobo_app_xml_merge (app, "/menu", simplec,
			      bonobo_object_corba_objref (BONOBO_OBJECT (componentc)));
	
	bonobo_app_xml_merge (app, "/menu/submenu/#file", simpled,
			      bonobo_object_corba_objref (BONOBO_OBJECT (componentc)));

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 2 ---\n\n\n");
	bonobo_app_xml_rm (app, "/", bonobo_object_corba_objref (BONOBO_OBJECT (componentb)));

	gtk_main ();

	fprintf (stderr, "\n\n--- Remove 3 ---\n\n\n");
	bonobo_app_xml_rm (app, "/", bonobo_object_corba_objref (BONOBO_OBJECT (componentc)));

	gtk_main ();

	bonobo_object_unref (BONOBO_OBJECT (componenta));
	bonobo_object_unref (BONOBO_OBJECT (componentb));
	bonobo_object_unref (BONOBO_OBJECT (componentc));

	bonobo_object_unref (BONOBO_OBJECT (app));

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
