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
		"	<menuitem name=\"close\" label=\"_CloseB\" pixtype=\"stock\" pixname=\"Menu_Close\"/>\n"
		"</submenu>\n";
	char simplec [] =
		"<submenu name=\"file\" label=\"_FileC\">\n"
		"	<menuitem name=\"open\" label=\"_OpenC\" pixtype=\"stock\" pixname=\"Menu_Open\"/>\n"
		"	<menuitem name=\"save\" label=\"_SaveC\" pixtype=\"stock\" pixname=\"Menu_Save As\"/>\n"
		"</submenu>\n";
	char simpled [] =
		"<menuitem name=\"save\" label=\"_SaveD\" pixtype=\"stock\" pixname=\"Menu_Save\"/>\n";
	char toola [] =
		"<dockitem name=\"toolbar\">\n"
		"	<toolitem type=\"radio\" name=\"foo1\" pixtype=\"stock\" pixname=\"Save\" label=\"Save\" desc=\"My tooltip\"/>\n"
		"	<toolitem type=\"toggle\" name=\"foo2\" pixtype=\"stock\" pixname=\"Save\" label=\"TogSave\" desc=\"My tooltip\"/>\n"
		"	<toolitem type=\"separator\" name=\"foo3\" pixtype=\"stock\" pixname=\"Save\" label=\"Separator\" desc=\"My tooltip\"/>\n"
		"	<toolitem type=\"std\" name=\"baa\" pixtype=\"stock\" pixname=\"Open\" label=\"baa\"/>\n"
		"</dockitem>";
	char toolb [] =
		"<dockitem name=\"toolbar\" look=\"icon\" relief=\"none\"/>\n";
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

/*	{
		GdkPixbuf *pixbuf;
		char      *xml;

		pixbuf = gdk_pixbuf_new_from_file ("stock_close.png");
		if (pixbuf) {
			xml = bonobo_ui_util_pixbuf_to_xml (pixbuf);
			printf ("Serializes to '%s'\n", xml);

			pixbuf = bonobo_ui_util_xml_to_pixbuf (xml);
		}
		}*/

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

	bonobo_app_xml_merge (app, "/menu", simplea, componenta);
	bonobo_app_xml_merge (app, "/",     toola,   componentb);

	help = bonobo_ui_build_help_menu (componenta, "gnomecal");
	bonobo_app_xml_merge_tree (app, "/menu", help, componenta);

	bonobo_app_xml_merge (app, "/menu", simpleb, componentb);
	bonobo_app_xml_merge (app, "/",     toolb,   componenta);

	bonobo_app_xml_merge (app, "/menu", simplec, componentc);
	
	bonobo_app_xml_merge (app, "/menu/submenu/#file", simpled, componentc);

	fprintf (stderr, "\n\n--- Remove 2 ---\n\n\n");
	bonobo_app_xml_rm (app, "/", componentb);

	fprintf (stderr, "\n\n--- Remove 3 ---\n\n\n");
	bonobo_app_xml_rm (app, "/", componentc);

	bonobo_object_unref (BONOBO_OBJECT (componenta));
	bonobo_object_unref (BONOBO_OBJECT (componentb));
	bonobo_object_unref (BONOBO_OBJECT (componentc));

	bonobo_object_unref (BONOBO_OBJECT (app));

	if (ctx)
		poptFreeContext (ctx);

	return 0;
}
