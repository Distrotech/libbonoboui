#ifndef _GNOME_COMPONENT_H_
#define _GNOME_COMPONENT_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>

BEGIN_GNOME_DECLS
 
#define GNOME_COMPONENT_TYPE        (gnome_component_get_type ())
#define GNOME_COMPONENT(o)          (GTK_CHECK_CAST ((o), GNOME_COMPONENT_TYPE, GnomeComponent))
#define GNOME_COMPONENT_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_COMPONENT_TYPE, GnomeComponentClass))
#define GNOME_IS_COMPONENT(o)       (GTK_CHECK_TYPE ((o), GNOME_COMPONENT_TYPE))
#define GNOME_IS_COMPONENT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_COMPONENT_TYPE))

struct _GnomeComponent;
typedef struct _GnomeComponent GnomeComponent;

typedef GnomeView * (*GnomeViewFactory)(GnomeComponent *component, void *closure);
					
struct _GnomeComponent {
	GnomeObject base;

	char *host_name;
	char *host_appname;
	GNOME_ClientSite client_site;

	/*
	 * The View factory
	 */
	GnomeViewFactory view_factory;
	void *view_factory_closure;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Signals
	 */
	void (*do_verb)            (GnomeComponent *comp,
				    CORBA_short verb,
				    const CORBA_char *verb_name);
	void (*host_name_changed)  (GnomeComponent *comp);
} GnomeComponentClass;

GtkType         gnome_component_get_type  (void);
GnomeComponent *gnome_component_new       (GnomeViewFactory factory,
					   void *data);
GnomeComponent *gnome_component_construct (GnomeComponent *component,
					   GNOME_Component  corba_component,
					   GnomeViewFactory factory,
					   void *data);

void     gnome_component_set_view_factory (GnomeComponent *component,
					   GnomeViewFactory factory,
					   void *data);

END_GNOME_DECLS

#endif
