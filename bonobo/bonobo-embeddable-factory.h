#ifndef _GNOME_BONOBO_OBJECT_FACTORY_H_
#define _GNOME_BONOBO_OBJECT_FACTORY_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>
#include <bonobo/gnome-component.h>

BEGIN_GNOME_DECLS
 
#define GNOME_BONOBO_OBJECT_FACTORY_TYPE        (gnome_bonobo_object_factory_get_type ())
#define GNOME_BONOBO_OBJECT_FACTORY(o)          (GTK_CHECK_CAST ((o), GNOME_BONOBO_OBJECT_FACTORY_TYPE, GnomeBonoboObjectFactory))
#define GNOME_BONOBO_OBJECT_FACTORY_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_BONOBO_OBJECT_FACTORY_TYPE, GnomeBonoboObjectFactoryClass))
#define GNOME_IS_BONOBO_OBJECT_FACTORY(o)       (GTK_CHECK_TYPE ((o), GNOME_BONOBO_OBJECT_FACTORY_TYPE))
#define GNOME_IS_BONOBO_OBJECT_FACTORY_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_BONOBO_OBJECT_FACTORY_TYPE))

struct _GnomeBonoboObjectFactory;
typedef struct _GnomeBonoboObjectFactory GnomeBonoboObjectFactory;

typedef GnomeBonoboObject * (*GnomeBonoboObjectFactoryFn)(GnomeBonoboObjectFactory *Factory, const char *path, void *closure);
					
struct _GnomeBonoboObjectFactory {
	GnomeObject base;

	/*
	 * The function factory
	 */
	GnomeBonoboObjectFactoryFn factory;
	void *factory_closure;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Virtual methods
	 */
	GnomeBonoboObject * (*new_bonobo_object)(GnomeBonoboObjectFactory *c_factory, const char *path);
} GnomeBonoboObjectFactoryClass;

GtkType                   gnome_bonobo_object_factory_get_type  (void);
GnomeBonoboObjectFactory *gnome_bonobo_object_factory_new       (const char *goad_id,
								 GnomeBonoboObjectFactoryFn factory,
								 void *data);
GnomeBonoboObjectFactory *gnome_bonobo_object_factory_construct (const char *goad_id,
								 GnomeBonoboObjectFactory *c_factory,
								 GNOME_BonoboObject       corba_factory,
								 GnomeBonoboObjectFactoryFn factory,
								 void *data);

void                      gnome_bonobo_object_factory_set       (GnomeBonoboObjectFactory *c_factory,
								 GnomeBonoboObjectFactoryFn factory,
								 void *data);

END_GNOME_DECLS

#endif
