/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __BONOBO_SOCKET_H__
#define __BONOBO_SOCKET_H__

#include <gtk/gtkcontainer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BONOBO_SOCKET(obj)          GTK_CHECK_CAST (obj, bonobo_socket_get_type (), BonoboSocket)
#define BONOBO_SOCKET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, bonobo_socket_get_type (), BonoboSocketClass)
#define BONOBO_IS_SOCKET(obj)       GTK_CHECK_TYPE (obj, bonobo_socket_get_type ())

typedef struct _BonoboSocket        BonoboSocket;
typedef struct _BonoboSocketClass   BonoboSocketClass;

struct _BonoboSocket {
	GtkContainer container;
	
	guint16 request_width;
	guint16 request_height;
	guint16 current_width;
	guint16 current_height;
	
	GdkWindow *plug_window;
	guint same_app : 1;
	guint focus_in : 1;
	guint have_size : 1;
	guint need_map : 1;
};

struct _BonoboSocketClass {
	GtkContainerClass parent_class;
};


GtkWidget*     bonobo_socket_new      (void);
guint          bonobo_socket_get_type (void);
void           bonobo_socket_steal    (BonoboSocket *socket,
				       guint32 wid);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BONOBO_SOCKET_H__ */