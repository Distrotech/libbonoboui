/* $Id */
/*
  Bonobo-Sample Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"
#include "container-menu.h"

#include "container-io.h"
#include "container-print.h"
#include "container-filesel.h"

static void add_cb (GtkWidget * caller, SampleApp * app);
static void load_cb (GtkWidget * caller, SampleApp * inst);
static void save_cb (GtkWidget * caller, SampleApp * inst);
static void exit_cb (GtkWidget * caller, SampleApp * inst);
static void about_cb (GtkWidget * caller, SampleApp * inst);
static void print_preview_cb (GtkWidget * caller, SampleApp * inst);

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
sample_app_fill_menu (SampleApp * app)
{
	BonoboUIHandlerMenuItem *menu_list;

	/* Load the menu bar with the container-specific base menus */
	menu_list = bonobo_ui_handler_menu_parse_uiinfo_list_with_data
	    (sample_app_menu, app);
	bonobo_ui_handler_menu_add_list (app->ui_handler, "/", menu_list);
	bonobo_ui_handler_menu_free_list (menu_list);
}

static void
add_cb (GtkWidget * caller, SampleApp * inst)
{
#if GUI
	char *required_interfaces[2] =
	    { "IDL:Bonobo/Embeddable:1.0", NULL };
	char *obj_id;

	/* Ask the user to select a component. */
#if USING_OAF
	obj_id =
	    gnome_bonobo_select_oaf_id (_
					("Select an embeddable Bonobo component to add"),
					(const gchar **) required_interfaces);
#else
	obj_id =
	    gnome_bonobo_select_goad_id (_
					 ("Select an embeddable Bonobo component to add"),
					 (const gchar **) required_interfaces);
#endif

	if (!obj_id)
		return;

	/* Activate it. */
	sample_app_add_component (inst, obj_id);

	g_free (obj_id);
#else
	sample_app_add_component (inst, OBJ_ID);
#endif
}

#if GUI

static void
load_ok_cb (GtkWidget * caller, SampleApp * app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename = gtk_file_selection_get_filename
	    (GTK_FILE_SELECTION (fs));

	if (filename)
		sample_container_load (app, filename);

	gtk_widget_destroy (fs);
}

static void
save_ok_cb (GtkWidget * caller, SampleApp * app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename = gtk_file_selection_get_filename
	    (GTK_FILE_SELECTION (fs));

	if (filename)
		sample_container_save (app, filename);

	gtk_widget_destroy (fs);
}
#endif


static void
save_cb (GtkWidget * caller, SampleApp * app)
{
#if GUI
	container_request_file (app, TRUE, save_ok_cb, app);
#else
	sample_container_save (app, FILE);
#endif
}

static void
load_cb (GtkWidget * caller, SampleApp * app)
{
#if GUI
	container_request_file (app, FALSE, load_ok_cb, app);
#else
	sample_container_load (app, FILE);
#endif
}

static void
print_preview_cb (GtkWidget * caller, SampleApp * app)
{
	sample_app_print_preview (app);
}

static void
about_cb (GtkWidget * caller, SampleApp * app)
{
	static const gchar *authors[] = {
		"ÉRDI Gergõ <cactus@cactus.rulez.org>",
		NULL
	};

	GtkWidget *about = gnome_about_new ("sample-container", VERSION,
					    "(C) 2000 ÉRDI Gergõ",
					    authors,
					    _
					    ("Bonobo demonstration container"),
					    NULL);
	gtk_widget_show (about);
}

static void
exit_cb (GtkWidget * caller, SampleApp * app)
{
	sample_app_exit (app);
}
