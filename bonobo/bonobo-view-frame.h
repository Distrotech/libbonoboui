#ifndef _GNOME_VIEW_FRAME_H_
#define _GNOME_VIEW_FRAME_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-client-site.h>
#include <bonobo/gnome-wrapper.h>

BEGIN_GNOME_DECLS
 
#define GNOME_VIEW_FRAME_TYPE        (gnome_view_frame_get_type ())
#define GNOME_VIEW_FRAME(o)          (GTK_CHECK_CAST ((o), GNOME_VIEW_FRAME_TYPE, GnomeViewFrame))
#define GNOME_VIEW_FRAME_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_VIEW_FRAME_TYPE, GnomeViewFrameClass))
#define GNOME_IS_VIEW_FRAME(o)       (GTK_CHECK_TYPE ((o), GNOME_VIEW_FRAME_TYPE))
#define GNOME_IS_VIEW_FRAME_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_VIEW_FRAME_TYPE))

typedef struct {
	GnomeUnknown base;

	GnomeWrapper    *wrapper; 

	GnomeClientSite *client_site;
} GnomeViewFrame;

typedef struct {
	GnomeUnknownClass parent_class;

	/*
	 * signal
	 */
	void (*view_activated) (GnomeViewFrame *view_frame, gboolean state);
} GnomeViewFrameClass;

GtkType           gnome_view_frame_get_type    (void);
GnomeViewFrame   *gnome_view_frame_construct   (GnomeViewFrame *view_frame,
						GNOME_ViewFrame corba_view_frame,
						GnomeWrapper   *wrapper,
						GnomeClientSite *client_site);
GnomeViewFrame   *gnome_view_frame_new         (GnomeClientSite *client_site);
GtkWidget        *gnome_view_frame_get_wrapper (GnomeViewFrame *view_frame);

END_GNOME_DECLS

#endif /* _GNOME_VIEW_FRAME_H_ */





