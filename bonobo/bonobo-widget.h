/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Bonobo Widget object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
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

struct _GnomeBonoboWidgetPrivate;
typedef struct _GnomeBonoboWidgetPrivate GnomeBonoboWidgetPrivate;

#include <bonobo/gnome-view.h>

struct _GnomeBonoboWidget {
	GtkBin		          bin;

	GnomeBonoboWidgetPrivate *priv;
};

typedef struct {
	GtkBinClass	 bin_class;
} GnomeBonoboWidgetClass;

GtkType            gnome_bonobo_widget_get_type                (void);
GnomeObjectClient *gnome_bonobo_widget_get_server              (GnomeBonoboWidget *bw);

/*
 * GnomeBonoboWidget for Controls.
 */
GtkWidget	  *gnome_bonobo_widget_new_control	       (char *goad_id);
GtkWidget	  *gnome_bonobo_widget_new_control_from_objref (GNOME_Control control);
GnomeControlFrame *gnome_bonobo_widget_get_control_frame       (GnomeBonoboWidget *bw);

/*
 * Gnome Bonobo Widget for subdocuments (Embeddables with a single View).
 */
GtkWidget	  *gnome_bonobo_widget_new_subdoc              (char *object_desc,
							       	GnomeUIHandler *uih);
GnomeContainer	  *gnome_bonobo_widget_get_container           (GnomeBonoboWidget *bw);
GnomeClientSite	  *gnome_bonobo_widget_get_client_site         (GnomeBonoboWidget *bw);
GnomeViewFrame	  *gnome_bonobo_widget_get_view_frame          (GnomeBonoboWidget *bw);
GnomeUIHandler	  *gnome_bonobo_widget_get_uih	               (GnomeBonoboWidget *bw);

END_GNOME_DECLS

#endif
