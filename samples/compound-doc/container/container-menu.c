#include "config.h"
#include "container-menu.h"

#include "container-io.h"
#include "container-print.h"
#include "container-filesel.h"

static void
add_cb (GtkWidget *caller, SampleApp *inst)
{
	char *required_interfaces [2] =
	    { "IDL:Bonobo/Embeddable:1.0", NULL };
	char *obj_id;

	/* Ask the user to select a component. */
	obj_id = bonobo_selector_select_id (
		_("Select an embeddable Bonobo component to add"),
		(const gchar **) required_interfaces);

	if (!obj_id)
		return;

	/* Activate it. */
	sample_app_add_component (inst, obj_id);

	g_free (obj_id);
}

static void
load_ok_cb (GtkWidget *caller, SampleApp *app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename = gtk_file_selection_get_filename
		(GTK_FILE_SELECTION (fs));

	if (filename)
		sample_container_load (app, filename);

	gtk_widget_destroy (fs);
}

static void
save_ok_cb (GtkWidget *caller, SampleApp *app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename = gtk_file_selection_get_filename
	    (GTK_FILE_SELECTION (fs));

	if (filename)
		sample_container_save (app, filename);

	gtk_widget_destroy (fs);
}

static void
save_cb (GtkWidget *caller, SampleApp *app)
{
	container_request_file (app, TRUE, save_ok_cb, app);
}

static void
load_cb (GtkWidget *caller, SampleApp *app)
{
	container_request_file (app, FALSE, load_ok_cb, app);
}

static void
print_preview_cb (GtkWidget *caller, SampleApp *app)
{
	sample_app_print_preview (app);
}

static void
xml_dump_cb (GtkWidget *caller, SampleApp *app)
{
	bonobo_win_dump (app->app, "On request");
}

static void
about_cb (GtkWidget *caller, SampleApp *app)
{
	static const gchar *authors[] = {
		"ÉRDI Gergõ <cactus@cactus.rulez.org>",
		"Michael Meeks <michael@helixcode.com>",
		NULL
	};

	GtkWidget *about = gnome_about_new ("sample-container", VERSION,
					    "(C) 2000 ÉRDI Gergõ, Helix Code, Inc",
					    authors,
					    _("Bonobo sample container"), NULL);
	gtk_widget_show (about);
}

static void
exit_cb (GtkWidget *caller, SampleApp *app)
{
	sample_app_exit (app);
}

/*
 * The menus.
 */
static GnomeUIInfo sample_app_file_menu[] = {
	GNOMEUIINFO_ITEM_NONE (N_("A_dd a new Embeddable component"), NULL,
			       add_cb),

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_OPEN_ITEM (load_cb, NULL),
	GNOMEUIINFO_MENU_SAVE_AS_ITEM (save_cb, NULL),
	BONOBOUIINFO_PLACEHOLDER ("Placeholder"),

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_NONE (N_("Xml dump"), NULL,
			       xml_dump_cb),

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_NONE (N_("Print Pre_view"), NULL,
			       print_preview_cb),

	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo sample_app_help_menu[] = {
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo sample_app_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (sample_app_file_menu),
	GNOMEUIINFO_MENU_HELP_TREE (sample_app_help_menu),
	GNOMEUIINFO_END
};

void
sample_app_fill_menu (SampleApp *app)
{
	BonoboUIHandlerMenuItem *menu_list;

	/* Load the menu bar with the container-specific base menus */
	menu_list = bonobo_ui_handler_menu_parse_uiinfo_list_with_data
		(sample_app_menu, app);

	bonobo_ui_handler_menu_add_list  (app->ui_handler, "/", menu_list);
	bonobo_ui_handler_menu_free_list (menu_list);
}
