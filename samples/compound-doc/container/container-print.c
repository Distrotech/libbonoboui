
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

#include "container-print.h"

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-master.h>
#include <libgnomeprint/gnome-print-master-preview.h>
#include "component.h"

void
sample_app_print_preview (SampleApp * app)
{
	GList *l;
	GnomePrintMaster *pm;
	GnomePrintContext *ctx;
	GnomePrintMasterPreview *pv;
	double ypos = 0.0;

	pm = gnome_print_master_new ();
	ctx = gnome_print_master_get_context (pm);

	for (l = app->components; l; l = l->next) {
		Component *component = l->data;
		component_print (component, ctx, 0.0, ypos, 100.0, 150.0);

		ypos += 150.0;
	}

	gnome_print_showpage (ctx);
	gnome_print_context_close (ctx);

	pv = gnome_print_master_preview_new (pm, "Component demo");
	gtk_widget_show (GTK_WIDGET (pv));
	gtk_main ();
	gtk_object_unref (GTK_OBJECT (pm));
}
