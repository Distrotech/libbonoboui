#ifndef _GNOME_BONOBO_OBJECT_H_
#define _GNOME_BONOBO_OBJECT_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>

BEGIN_GNOME_DECLS
 
#define GNOME_BONOBO_OBJECT_TYPE        (gnome_bonobo_object_get_type ())
#define GNOME_BONOBO_OBJECT(o)          (GTK_CHECK_CAST ((o), GNOME_BONOBO_OBJECT_TYPE, GnomeBonoboObject))
#define GNOME_BONOBO_OBJECT_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_BONOBO_OBJECT_TYPE, GnomeBonoboObjectClass))
#define GNOME_IS_BONOBO_OBJECT(o)       (GTK_CHECK_TYPE ((o), GNOME_BONOBO_OBJECT_TYPE))
#define GNOME_IS_BONOBO_OBJECT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_BONOBO_OBJECT_TYPE))

struct _GnomeBonoboObject;
typedef struct _GnomeBonoboObject GnomeBonoboObject;

typedef GnomeView * (*GnomeViewFactory)(GnomeBonoboObject *bonobo_object, const GNOME_ViewFrame view_frame, void *closure);
					
struct _GnomeBonoboObject {
	GnomeObject base;

	char *host_name;
	char *host_appname;
	GNOME_ClientSite client_site;

	/*
	 * The View factory
	 */
	GnomeViewFactory view_factory;
	void *view_factory_closure;

	/*
	 * The verbs
	 */
	GList *verbs;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*host_name_changed)  (GnomeBonoboObject *comp);
} GnomeBonoboObjectClass;

GtkType            gnome_bonobo_object_get_type           (void);
GnomeBonoboObject *gnome_bonobo_object_new                (GnomeViewFactory factory,
							   void *data);
GnomeBonoboObject *gnome_bonobo_object_construct          (GnomeBonoboObject *bonobo_object,
							   GNOME_BonoboObject  corba_bonobo_object,
							   GnomeViewFactory factory,
							   void *data);
void               gnome_bonobo_object_add_verb           (GnomeBonoboObject *bonobo_object,
							   const char *verb_name);
void               gnome_bonobo_object_add_verbs          (GnomeBonoboObject *bonobo_object,
							   const char **verb_list);
void               gnome_bonobo_object_remove_verb        (GnomeBonoboObject *bonobo_object,
							   const char *verb_name);
void               gnome_bonobo_object_set_view_factory   (GnomeBonoboObject *bonobo_object,
							   GnomeViewFactory factory,
							   void *data);

END_GNOME_DECLS

#endif


