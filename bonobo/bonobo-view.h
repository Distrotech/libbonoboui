/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_VIEW_H_
#define _BONOBO_VIEW_H_

#include <libgnome/gnome-defs.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-view-frame.h>

BEGIN_GNOME_DECLS
 
#define BONOBO_VIEW_TYPE        (bonobo_view_get_type ())
#define BONOBO_VIEW(o)          (GTK_CHECK_CAST ((o), BONOBO_VIEW_TYPE, BonoboView))
#define BONOBO_VIEW_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_VIEW_TYPE, BonoboViewClass))
#define BONOBO_IS_VIEW(o)       (GTK_CHECK_TYPE ((o), BONOBO_VIEW_TYPE))
#define BONOBO_IS_VIEW_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_VIEW_TYPE))

typedef struct _BonoboView BonoboView;
typedef struct _BonoboViewPrivate BonoboViewPrivate;
typedef struct _BonoboViewClass BonoboViewClass;

#include <bonobo/bonobo-embeddable.h>

#define BONOBO_VIEW_VERB_FUNC(fn) ((BonoboViewVerbFunc)(fn))
typedef void (*BonoboViewVerbFunc)(BonoboView *view, const char *verb_name, void *user_data);

struct _BonoboView {
	BonoboControl base;

	BonoboEmbeddable  *embeddable;
	Bonobo_ViewFrame   view_frame;
	BonoboViewPrivate *priv;
};

struct _BonoboViewClass {
	BonoboControlClass parent_class;

	/*
	 * Signals
	 */
	void (*do_verb)                  (BonoboView *view,
					  const CORBA_char *verb_name);
	void (*set_zoom_factor)          (BonoboView *view, double zoom);
};

GtkType               bonobo_view_get_type               (void);
BonoboView           *bonobo_view_construct              (BonoboView         *view,
							  Bonobo_View         corba_view,
							  GtkWidget          *widget);
BonoboView           *bonobo_view_new                    (GtkWidget          *widget);
Bonobo_View           bonobo_view_corba_object_create    (BonoboObject       *object);
void                  bonobo_view_set_embeddable         (BonoboView         *view,
							  BonoboEmbeddable   *embeddable);
BonoboEmbeddable     *bonobo_view_get_embeddable         (BonoboView         *view);
void                  bonobo_view_set_view_frame         (BonoboView         *view,
							  Bonobo_ViewFrame    view_frame);
Bonobo_ViewFrame      bonobo_view_get_view_frame         (BonoboView         *view);
Bonobo_Unknown        bonobo_view_get_remote_ui_handler  (BonoboView         *view);
BonoboUIHandler      *bonobo_view_get_ui_handler         (BonoboView         *view);
void                  bonobo_view_activate_notify        (BonoboView         *view,
							  gboolean            activated);
						  
/* Verbs. */					  
void                  bonobo_view_register_verb          (BonoboView         *view,
							  const char         *verb_name,
							  BonoboViewVerbFunc  callback,
							  gpointer            user_data);
void                  bonobo_view_unregister_verb        (BonoboView         *view,
							  const char         *verb_name);
void                  bonobo_view_execute_verb           (BonoboView         *view,
							  const char         *verb_name);
char                 *bonobo_view_popup_verbs            (BonoboView         *view);
POA_Bonobo_View__epv *bonobo_view_get_epv                (void);

END_GNOME_DECLS

#endif /* _BONOBO_VIEW_H_ */
