/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_APP_H_
#define _BONOBO_APP_H_

#include <bonobo/bonobo-object.h>
#include <gnome.h>

/*
 * FIXME:
 * bonobo-widget.c: 257
 * bonobo-control.c: 190, 693
 * bonobo-control-frame.c: 359
 * bonobo-view.c: 578
 * bonobo-view-frame.c: 564
 */

#define BONOBO_APP_TYPE        (bonobo_app_get_type ())
#define BONOBO_APP(o)          (GTK_CHECK_CAST ((o), BONOBO_APP_TYPE, BonoboApp))
#define BONOBO_APP_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_APP_TYPE, BonoboAppClass))
#define BONOBO_IS_APP(o)       (GTK_CHECK_TYPE ((o), BONOBO_APP_TYPE))
#define BONOBO_IS_APP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_APP_TYPE))

typedef struct _BonoboApp        BonoboApp;
typedef struct _BonoboAppPrivate BonoboAppPrivate;
typedef struct _BonoboAppClass   BonoboAppClass;

struct _BonoboApp {
	BonoboObject      base;
	BonoboAppPrivate *priv;
};

struct _BonoboAppClass {
	BonoboObjectClass parent;
};

GtkType                      bonobo_app_get_type            (void);
POA_Bonobo_UIContainer__epv *bonobo_app_get_epv             (void);
Bonobo_UIContainer           bonobo_app_corba_object_create (BonoboObject      *object);
BonoboApp                   *bonobo_app_construct           (BonoboApp         *app,
							     Bonobo_UIContainer corba_app,
							     const char        *app_name,
							     const char        *title);

BonoboApp           *bonobo_app_new                 (const char *app_name,
						     const char *title);

void                 bonobo_app_set_contents        (BonoboApp  *app,
						     GtkWidget  *contents);
GtkWidget           *bonobo_app_get_contents        (BonoboApp  *app);
GtkWidget           *bonobo_app_get_window          (BonoboApp  *app);

gboolean             bonobo_app_xml_merge           (BonoboApp  *app,
						     const char *path,
						     const char *xml,
						     gpointer    listener);

void                 bonobo_app_xml_merge_tree      (BonoboApp  *app,
						     const char *path,
						     xmlNode    *tree,
						     gpointer    listener);

void                 bonobo_app_xml_rm              (BonoboApp  *app,
						     const char *path,
						     gpointer    by_listner);

GtkAccelGroup       *bonobo_app_get_accel_group     (BonoboApp  *app);

void                 bonobo_app_dump                (BonoboApp  *app,
						     const char *msg);

/* Compat function; may disappear any second now. */
BonoboApp           *bonobo_app_from_window         (GtkWindow *window);

#endif /* _BONOBO_APP_H_ */
