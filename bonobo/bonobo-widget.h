/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Bonobo Widget object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#ifndef _GNOME_BONOBO_WIDGET_H_
#define _GNOME_BONOBO_WIDGET_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-wrapper.h>

BEGIN_GNOME_DECLS
 
#define GNOME_BONOBO_WIDGET_TYPE        (gnome_bonobo_widget_get_type ())
#define GNOME_BONOBO_WIDGET(o)          (GTK_CHECK_CAST ((o), GNOME_BONOBO_WIDGET_TYPE, GnomeBonoboWidget))
#define GNOME_BONOBO_WIDGET_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_BONOBO_WIDGET_TYPE, GnomeBonoboWidgetClass))
#define GNOME_IS_BONOBO_WIDGET(o)       (GTK_CHECK_TYPE ((o), GNOME_BONOBO_WIDGET_TYPE))
#define GNOME_IS_BONOBO_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_BONOBO_WIDGET_TYPE))

struct _GnomeBonoboWidget;
typedef struct _GnomeBonoboWidget GnomeBonoboWidget;

#include <bonobo/gnome-view.h>

struct _GnomeBonoboWidget {
	GtkBin		   bin;

	GnomeObjectClient *server;
	GnomeContainer	  *container;
	GnomeClientSite   *client_site;
	GnomeViewFrame    *view_frame;

	GnomeUIHandler	  *uih;
};

typedef struct {
	GtkBinClass	 bin_class;
} GnomeBonoboWidgetClass;

GtkType            gnome_bonobo_widget_get_type       (void);
GnomeBonoboWidget *gnome_bonobo_widget_new            (char *object_desc,
						       GnomeUIHandler *uih);

END_GNOME_DECLS

#endif
