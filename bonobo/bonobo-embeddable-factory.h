#ifndef _GNOME_COMPONENT_FACTORY_H_
#define _GNOME_COMPONENT_FACTORY_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>
#include <bonobo/gnome-embeddable.h>

BEGIN_GNOME_DECLS
 
#define GNOME_EMBEDDABLE_FACTORY_TYPE        (gnome_embeddable_factory_get_type ())
#define GNOME_EMBEDDABLE_FACTORY(o)          (GTK_CHECK_CAST ((o), GNOME_EMBEDDABLE_FACTORY_TYPE, GnomeEmbeddableFactory))
#define GNOME_EMBEDDABLE_FACTORY_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_EMBEDDABLE_FACTORY_TYPE, GnomeEmbeddableFactoryClass))
#define GNOME_IS_EMBEDDABLE_FACTORY(o)       (GTK_CHECK_TYPE ((o), GNOME_EMBEDDABLE_FACTORY_TYPE))
#define GNOME_IS_EMBEDDABLE_FACTORY_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_EMBEDDABLE_FACTORY_TYPE))

struct _GnomeEmbeddableFactory;
typedef struct _GnomeEmbeddableFactory GnomeEmbeddableFactory;

typedef GnomeObject * (*GnomeEmbeddableFactoryFn)(GnomeEmbeddableFactory *Factory, void *closure);
					
struct _GnomeEmbeddableFactory {
	GnomeObject base;

	/*
	 * The function factory
	 */
	GnomeEmbeddableFactoryFn factory;
	void *factory_closure;
};

typedef struct {
	GnomeObjectClass parent_class;

	/*
	 * Virtual methods
	 */
	GnomeObject * (*new_embeddable)(GnomeEmbeddableFactory *c_factory);
} GnomeEmbeddableFactoryClass;

GtkType gnome_embeddable_factory_get_type  (void);

GnomeEmbeddableFactory *gnome_embeddable_factory_new (
	const char *goad_id,
	GnomeEmbeddableFactoryFn factory,
	void *data);

GnomeEmbeddableFactory *gnome_embeddable_factory_construct (
	const char *goad_id,
	GnomeEmbeddableFactory  *c_factory,
	GNOME_GenericFactory    corba_factory,
	GnomeEmbeddableFactoryFn factory,
	void *data);

void gnome_embeddable_factory_set (
	GnomeEmbeddableFactory *c_factory,
	GnomeEmbeddableFactoryFn factory,
	void *data);

END_GNOME_DECLS

#endif
