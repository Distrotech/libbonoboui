/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _GNOME_VIEW_H_
#define _GNOME_VIEW_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view-frame.h>
#include <bonobo/gnome-ui-handler.h>

BEGIN_GNOME_DECLS
 
#define GNOME_VIEW_TYPE        (gnome_view_get_type ())
#define GNOME_VIEW(o)          (GTK_CHECK_CAST ((o), GNOME_VIEW_TYPE, GnomeView))
#define GNOME_VIEW_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_VIEW_TYPE, GnomeViewClass))
#define GNOME_IS_VIEW(o)       (GTK_CHECK_TYPE ((o), GNOME_VIEW_TYPE))
#define GNOME_IS_VIEW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_VIEW_TYPE))

typedef struct _GnomeView GnomeView;
typedef struct _GnomeViewClass GnomeViewClass;

#include <bonobo/gnome-embeddable.h>

typedef void (*GnomeViewVerbFunc)(GnomeView *view, const char *verb_name, void *user_data);

struct _GnomeView {
	GnomeObject base;

	GtkWidget *widget;
	GtkWidget *plug;

	int plug_destroy_id;

	GNOME_ViewFrame view_frame;

	GnomeUIHandler *uih;

	GnomeEmbeddable *embeddable;

	GHashTable *verb_callbacks;
	GHashTable *verb_callback_closures;
};

struct _GnomeViewClass {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*view_activate)            (GnomeView *view,
					  gboolean activate);
	void (*view_undo_last_operation) (GnomeView *view);
	void (*size_query)               (GnomeView *view, int *desired_width, int *desired_height);
	void (*do_verb)                  (GnomeView *view,
					  const CORBA_char *verb_name);
	void (*set_zoom_factor)          (GnomeView *view, double zoom);
};

GtkType		 gnome_view_get_type		(void);
GnomeView	*gnome_view_construct		(GnomeView *view,
						 GNOME_View corba_view,
						 GtkWidget *widget);
GnomeView	*gnome_view_new                 (GtkWidget *widget);
GNOME_View	 gnome_view_corba_object_create	(GnomeObject *object);
void		 gnome_view_set_embeddable	(GnomeView *view,
						 GnomeEmbeddable *embeddable);
GnomeEmbeddable *gnome_view_get_embeddable	(GnomeView *view);
void		 gnome_view_set_view_frame	(GnomeView *view,
						 GNOME_ViewFrame view_frame);
GNOME_ViewFrame  gnome_view_get_view_frame	(GnomeView *view);
GNOME_UIHandler  gnome_view_get_remote_ui_handler(GnomeView *view);
void		 gnome_view_activate_notify	(GnomeView *view, gboolean activated);
void		 gnome_view_set_ui_handler	(GnomeView *view, GnomeUIHandler *uih);
GnomeUIHandler	*gnome_view_get_ui_handler	(GnomeView *view);
void		 gnome_view_register_verb	(GnomeView *view,
						 const char *verb_name,
						 GnomeViewVerbFunc callback,
						 gpointer user_data);
void		 gnome_view_unregister_verb	(GnomeView *view,
						 const char *verb_name);
void		 gnome_view_request_resize	(GnomeView *view,
						 int width, int height);

/* CORBA default vector methods we provide */
extern POA_GNOME_View__epv gnome_view_epv;
extern POA_GNOME_View__vepv gnome_view_vepv;

END_GNOME_DECLS

#endif /* _GNOME_VIEW_H_ */
