/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_VIEW_FRAME_H_
#define _GNOME_VIEW_FRAME_H_

#include <libgnome/gnome-defs.h>
#include <bonobo/gnome-control-frame.h>
#include <bonobo/gnome-ui-handler.h>

BEGIN_GNOME_DECLS
 
#define GNOME_VIEW_FRAME_TYPE        (gnome_view_frame_get_type ())
#define GNOME_VIEW_FRAME(o)          (GTK_CHECK_CAST ((o), GNOME_VIEW_FRAME_TYPE, GnomeViewFrame))
#define GNOME_VIEW_FRAME_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_VIEW_FRAME_TYPE, GnomeViewFrameClass))
#define GNOME_IS_VIEW_FRAME(o)       (GTK_CHECK_TYPE ((o), GNOME_VIEW_FRAME_TYPE))
#define GNOME_IS_VIEW_FRAME_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_VIEW_FRAME_TYPE))

typedef struct _GnomeViewFramePrivate GnomeViewFramePrivate;
typedef struct _GnomeViewFrame GnomeViewFrame;

#include <bonobo/gnome-client-site.h>

struct _GnomeViewFrame {
	GnomeControlFrame	 base;
	GnomeViewFramePrivate	*priv;
};

typedef struct {
	GnomeControlFrameClass parent_class;

	/*
	 * Signals.
	 */
	void (*view_activated)      (GnomeViewFrame *view_frame, gboolean state);
	void (*undo_last_operation) (GnomeViewFrame *view_frame);
	void (*user_activate)       (GnomeViewFrame *view_frame);
	void (*user_context)        (GnomeViewFrame *view_frame);
} GnomeViewFrameClass;

GtkType           gnome_view_frame_get_type        (void);
GnomeViewFrame   *gnome_view_frame_construct       (GnomeViewFrame *view_frame,
						    GNOME_ViewFrame corba_view_frame,
						    GnomeClientSite *client_site);
GnomeViewFrame   *gnome_view_frame_new             (GnomeClientSite *client_site);
void		  gnome_view_frame_bind_to_view	   (GnomeViewFrame *view_frame,
						    GNOME_View view);
GNOME_View	  gnome_view_frame_get_view	   (GnomeViewFrame *view_frame);

void		  gnome_view_frame_set_ui_handler  (GnomeViewFrame *view_frame,
						    GnomeUIHandler *uih);
GnomeUIHandler   *gnome_view_frame_get_ui_handler  (GnomeViewFrame *view_frame);
GnomeClientSite  *gnome_view_frame_get_client_site (GnomeViewFrame *view_frame);

GtkWidget        *gnome_view_frame_get_wrapper     (GnomeViewFrame *view_frame);

char		 *gnome_view_frame_popup_verbs	   (GnomeViewFrame *view_frame);

void		  gnome_view_frame_set_covered     (GnomeViewFrame *view_frame,
						    gboolean covered);
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
void              gnome_view_frame_set_zoom_factor (GnomeViewFrame *view_frame, double zoom);

POA_GNOME_ViewFrame__epv *gnome_view_frame_get_epv (void);

/* The entry point vectors for the server we provide */
extern POA_GNOME_ViewFrame__vepv gnome_view_frame_vepv;

END_GNOME_DECLS

#endif /* _GNOME_VIEW_FRAME_H_ */
