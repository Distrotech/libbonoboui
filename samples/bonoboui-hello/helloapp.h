/* app.h
 *
 * Copyright (C) 1999 Havoc Pennington, 2001 Murray Cumming
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*** gnomehello-apph */

#ifndef GNOMEHELLO_APP_H
#define GNOMEHELLO_APP_H

#include <config.h>
#include <bonobo.h>
#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-i18n.h>



GtkWidget* helloapp_new(const gchar* message, const gchar* geometry);
/* called from main() and helloapp_on_menu_file_new()*/
                         
#endif

/* gnomehello-apph ***/
