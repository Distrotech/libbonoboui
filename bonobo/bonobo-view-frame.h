/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_VIEW_FRAME_H_
#define _GNOME_VIEW_FRAME_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-wrapper.h>
#include <bonobo/gnome-ui-handler.h>

BEGIN_GNOME_DECLS
 
#define GNOME_VIEW_FRAME_TYPE        (gnome_view_frame_get_type ())
#define GNOME_VIEW_FRAME(o)          (GTK_CHECK_CAST ((o), GNOME_VIEW_FRAME_TYPE, GnomeViewFrame))
#define GNOME_VIEW_FRAME_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_VIEW_FRAME_TYPE, GnomeViewFrameClass))
#define GNOME_IS_VIEW_FRAME(o)       (GTK_CHECK_TYPE ((o), GNOME_VIEW_FRAME_TYPE))
#define GNOME_IS_VIEW_FRAME_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_VIEW_FRAME_TYPE))

typedef struct _GnomeViewFrame GnomeViewFrame ;

#include <bonobo/gnome-client-site.h>

struct _GnomeViewFrame {
	GnomeObject base;

	GnomeWrapper    *wrapper; 

	GnomeClientSite *client_site;

	GNOME_View	 view;

	GnomeUIHandler  *uih;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * signal
	 */
	void (*view_activated) (GnomeViewFrame *view_frame, gboolean state);
	void (*user_activate)  (GnomeViewFrame *view_frame);
} GnomeViewFrameClass;

GtkType           gnome_view_frame_get_type        (void);
GnomeViewFrame   *gnome_view_frame_construct       (GnomeViewFrame *view_frame,
						    GNOME_ViewFrame corba_view_frame,
						    GnomeWrapper   *wrapper,
						    GnomeClientSite *client_site);
GnomeViewFrame   *gnome_view_frame_new             (GnomeClientSite *client_site);

void		  gnome_view_frame_bind_to_view	   (GnomeViewFrame *view_frame,
						    GNOME_View view);
GNOME_View	  gnome_view_frame_get_view	   (GnomeViewFrame *view_frame);

/*
 * A GnomeViewFrame acts as a proxy for the remote GnomeView object to
 * which it is bound.  These functions act as wrappers which a
 * container can use to communicate with the GnomeView associated with
 * a given GnomeViewFrame.
 */
void		  gnome_view_frame_view_activate   (GnomeViewFrame *view_frame);
void		  gnome_view_frame_view_deactivate (GnomeViewFrame *view_frame);

void		  gnome_view_frame_view_do_verb	   (GnomeViewFrame *view_frame,
						    char *verb_name);


void		  gnome_view_frame_set_covered     (GnomeViewFrame *view_frame,
						    gboolean covered);

void		  gnome_view_frame_set_ui_handler  (GnomeViewFrame *view_frame,
						    GnomeUIHandler *uih);
GnomeUIHandler   *gnome_view_frame_get_ui_handler  (GnomeViewFrame *view_frame);

GtkWidget        *gnome_view_frame_get_wrapper     (GnomeViewFrame *view_frame);

END_GNOME_DECLS

#endif /* _GNOME_VIEW_FRAME_H_ */
