/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Bonobo Widget object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _BONOBO_WIDGET_H_
#define _BONOBO_WIDGET_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-wrapper.h>

BEGIN_GNOME_DECLS
 
#define BONOBO_WIDGET_TYPE        (bonobo_widget_get_type ())
#define BONOBO_WIDGET(o)          (GTK_CHECK_CAST ((o), BONOBO_WIDGET_TYPE, BonoboWidget))
#define BONOBO_WIDGET_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_WIDGET_TYPE, BonoboWidgetClass))
#define BONOBO_IS_WIDGET(o)       (GTK_CHECK_TYPE ((o), BONOBO_WIDGET_TYPE))
#define BONOBO_IS_WIDGET_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_WIDGET_TYPE))

struct _BonoboWidget;
typedef struct _BonoboWidget BonoboWidget;

struct _BonoboWidgetPrivate;
typedef struct _BonoboWidgetPrivate BonoboWidgetPrivate;

#include <bonobo/bonobo-view.h>

struct _BonoboWidget {
	GtkBin		          bin;

	BonoboWidgetPrivate *priv;
};

typedef struct {
	GtkBinClass	 bin_class;
} BonoboWidgetClass;

GtkType             bonobo_widget_get_type                 (void);
BonoboObjectClient *bonobo_widget_get_server               (BonoboWidget     *bw);

/*
 * BonoboWidget for Controls.
 */
GtkWidget          *bonobo_widget_new_control              (const char       *goad_id,
							    Bonobo_UIHandler  uih);
GtkWidget          *bonobo_widget_new_control_from_objref  (Bonobo_Control    control,
							    Bonobo_UIHandler  uih);
BonoboControlFrame *bonobo_widget_get_control_frame        (BonoboWidget     *bw);

/*
 * Gnome Bonobo Widget for subdocuments (Embeddables with a single View).
 */
GtkWidget          *bonobo_widget_new_subdoc               (const char       *object_desc,
							    Bonobo_UIHandler  uih);
BonoboContainer    *bonobo_widget_get_container            (BonoboWidget     *bw);
BonoboClientSite   *bonobo_widget_get_client_site          (BonoboWidget     *bw);
BonoboViewFrame    *bonobo_widget_get_view_frame           (BonoboWidget     *bw);
Bonobo_UIHandler    bonobo_widget_get_uih                  (BonoboWidget     *bw);

void                bonobo_widget_set_property            (BonoboWidget      *control,
							   const char        *first_prop,
							   ...);
void                bonobo_widget_get_property            (BonoboWidget      *control,
							   const char        *first_prop,
							   ...);

END_GNOME_DECLS

#endif
