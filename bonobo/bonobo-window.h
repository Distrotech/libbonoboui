/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_APP_H_
#define _BONOBO_APP_H_

#include <gnome.h>
#include <bonobo/bonobo-object.h>

#define BONOBO_APP_TYPE        (bonobo_app_get_type ())
#define BONOBO_APP(o)          (GTK_CHECK_CAST ((o), BONOBO_APP_TYPE, BonoboApp))
#define BONOBO_APP_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_APP_TYPE, BonoboAppClass))
#define BONOBO_IS_APP(o)       (GTK_CHECK_TYPE ((o), BONOBO_APP_TYPE))
#define BONOBO_IS_APP_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_APP_TYPE))

typedef struct _BonoboApp        BonoboApp;
typedef struct _BonoboAppPrivate BonoboAppPrivate;
typedef struct _BonoboAppClass   BonoboAppClass;

struct _BonoboApp {
	GtkWindow          parent;
	
	BonoboAppPrivate  *priv;
};

struct _BonoboAppClass {
	GtkWindowClass    parent_class;
};

GtkType              bonobo_app_get_type            (void);

GtkWidget           *bonobo_app_new                 (const char *app_name,
						     const char *title);

void                 bonobo_app_set_contents        (BonoboApp  *app,
						     GtkWidget  *contents);
GtkWidget           *bonobo_app_get_contents        (BonoboApp  *app);

gboolean             bonobo_app_xml_merge           (BonoboApp  *app,
						     const char *path,
						     const char *xml,
						     const char *component);

void                 bonobo_app_xml_merge_tree      (BonoboApp  *app,
						     const char *path,
						     xmlNode    *tree,
						     const char *component);

char                *bonobo_app_xml_get             (BonoboApp  *app,
						     const char *path,
						     gboolean    node_only);

gboolean             bonobo_app_xml_node_exists     (BonoboApp  *app,
						     const char *path);

void                 bonobo_app_xml_rm              (BonoboApp  *app,
						     const char *path,
						     const char *by_component);

void                 bonobo_app_object_set          (BonoboApp  *app,
						     const char *path,
						     Bonobo_Unknown object,
						     CORBA_Environment *ev);

Bonobo_Unknown       bonobo_app_object_get          (BonoboApp  *app,
						     const char *path,
						     CORBA_Environment *ev);

GtkAccelGroup       *bonobo_app_get_accel_group     (BonoboApp  *app);

void                 bonobo_app_dump                (BonoboApp  *app,
						     const char *msg);

void                 bonobo_app_register_component   (BonoboApp     *app,
						      const char    *name,
						      Bonobo_Unknown component);

void                 bonobo_app_deregister_component (BonoboApp     *app,
						      const char    *name);

Bonobo_Unknown       bonobo_app_component_get        (BonoboApp     *app,
						      const char    *name);

#endif /* _BONOBO_APP_H_ */
