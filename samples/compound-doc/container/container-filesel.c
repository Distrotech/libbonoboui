
/* $Id$ */
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

#include "container-filesel.h"

static void
cancel_cb (GtkWidget * caller, GtkWidget * fs)
{
	gtk_widget_destroy (fs);
}

void
container_request_file (SampleApp * app,
			gboolean save,
			GtkSignalFunc cb, gpointer user_data)
{
	GtkWidget *fs;

	app->fileselection = fs =
	    gtk_file_selection_new (_("Select file"));

	if (save)
		gtk_file_selection_show_fileop_buttons (GTK_FILE_SELECTION
							(fs));
	else
		gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION
							(fs));

	gtk_signal_connect (GTK_OBJECT
			    (GTK_FILE_SELECTION (fs)->ok_button),
			    "clicked", cb, user_data);
	gtk_signal_connect (GTK_OBJECT
			    (GTK_FILE_SELECTION (fs)->cancel_button),
			    "clicked", cancel_cb, fs);
	gtk_window_set_modal (GTK_WINDOW (fs), TRUE);
	gtk_widget_show (fs);
}
