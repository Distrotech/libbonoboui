/* $Id$ */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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

#include "hello-object-print.h"

void
hello_object_print (GnomePrintContext * ctx,
		    double width,
		    double height,
		    const Bonobo_PrintScissor * scissor,
		    gpointer user_data)
{
	Hello *obj = (Hello *) user_data;

	GnomeFont *font;
	double w, w2, h;
	const char str[] = "Component data:";

	gnome_print_setlinewidth (ctx, 2);
	font = gnome_font_new ("Helvetica", 12.0);
	g_return_if_fail (font != NULL);
	gnome_print_setrgbcolor (ctx, 0.0, 0.0, 0.0);
	gnome_print_setfont (ctx, font);

	w = gnome_font_get_width_string (font, str);
	w2 = gnome_font_get_width_string (font, obj->text);
	h =
	    gnome_font_get_ascender (font) +
	    gnome_font_get_descender (font);

	gnome_print_moveto (ctx, (width / 2) - (w / 2), (height / 2) + h);
	gnome_print_show (ctx, str);
	gnome_print_moveto (ctx, (width / 2) - (w2 / 2), height / 2);
	gnome_print_show (ctx, obj->text);

	gtk_object_unref (GTK_OBJECT (font));

#if 0
	/* X marks the spot */
	gnome_print_moveto (ctx, 0, 0);
	gnome_print_lineto (ctx, width, height);
	gnome_print_stroke (ctx);
#endif
}
