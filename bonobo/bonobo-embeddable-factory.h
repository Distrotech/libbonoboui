#ifndef _GNOME_COMPONENT_FACTORY_H_
#define _GNOME_COMPONENT_FACTORY_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>
#include <bonobo/gnome-embeddable.h>

BEGIN_GNOME_DECLS
 
#define GNOME_COMPONENT_FACTORY_TYPE        (gnome_component_factory_get_type ())
#define GNOME_COMPONENT_FACTORY(o)          (GTK_CHECK_CAST ((o), GNOME_COMPONENT_FACTORY_TYPE, GnomeComponentFactory))
#define GNOME_COMPONENT_FACTORY_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_COMPONENT_FACTORY_TYPE, GnomeComponentFactoryClass))
#define GNOME_IS_COMPONENT_FACTORY(o)       (GTK_CHECK_TYPE ((o), GNOME_COMPONENT_FACTORY_TYPE))
#define GNOME_IS_COMPONENT_FACTORY_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_COMPONENT_FACTORY_TYPE))

struct _GnomeComponentFactory;
typedef struct _GnomeComponentFactory GnomeComponentFactory;

typedef GnomeObject * (*GnomeComponentFactoryFn)(GnomeComponentFactory *Factory, void *closure);
					
struct _GnomeComponentFactory {
	GnomeObject base;

	/*
	 * The function factory
	 */
	GnomeComponentFactoryFn factory;
	void *factory_closure;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Virtual methods
	 */
	GnomeObject * (*new_component)(GnomeComponentFactory *c_factory);
} GnomeComponentFactoryClass;

GtkType gnome_component_factory_get_type  (void);

GnomeComponentFactory *gnome_component_factory_new (
	const char *goad_id,
	GnomeComponentFactoryFn factory,
	void *data);

GnomeComponentFactory *gnome_component_factory_construct (
	const char *goad_id,
	GnomeComponentFactory  *c_factory,
	GNOME_GenericFactory    corba_factory,
	GnomeComponentFactoryFn factory,
	void *data);

void gnome_component_factory_set (
	GnomeComponentFactory *c_factory,
	GnomeComponentFactoryFn factory,
	void *data);

END_GNOME_DECLS

#endif
