#ifndef _GNOME_EMBEDDABLE_H_
#define _GNOME_EMBEDDABLE_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>

BEGIN_GNOME_DECLS
 
#define GNOME_EMBEDDABLE_TYPE        (gnome_embeddable_get_type ())
#define GNOME_EMBEDDABLE(o)          (GTK_CHECK_CAST ((o), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddable))
#define GNOME_EMBEDDABLE_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_EMBEDDABLE_TYPE, GnomeEmbeddableClass))
#define GNOME_IS_EMBEDDABLE(o)       (GTK_CHECK_TYPE ((o), GNOME_EMBEDDABLE_TYPE))
#define GNOME_IS_EMBEDDABLE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_EMBEDDABLE_TYPE))

struct _GnomeEmbeddable;
typedef struct _GnomeEmbeddable GnomeEmbeddable;

typedef GnomeView * (*GnomeViewFactory)(GnomeEmbeddable *bonobo_object, const GNOME_ViewFrame view_frame, void *closure);
					
struct _GnomeEmbeddable {
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
	void (*host_name_changed)  (GnomeEmbeddable *comp);
} GnomeEmbeddableClass;

GtkType          gnome_embeddable_get_type         (void);
GnomeEmbeddable *gnome_embeddable_new              (GnomeViewFactory factory,
						    void *data);
GnomeEmbeddable *gnome_embeddable_construct        (GnomeEmbeddable *bonobo_object,
						    GNOME_Embeddable  corba_bonobo_object,
						    GnomeViewFactory factory,
						    void *data);
void             gnome_embeddable_add_verb         (GnomeEmbeddable *bonobo_object,
						    const char *verb_name);
void             gnome_embeddable_add_verbs        (GnomeEmbeddable *bonobo_object,
						    const char **verb_list);
void             gnome_embeddable_remove_verb      (GnomeEmbeddable *bonobo_object,
						    const char *verb_name);
void             gnome_embeddable_set_view_factory (GnomeEmbeddable *bonobo_object,
						    GnomeViewFactory factory,
						    void *data);

END_GNOME_DECLS

#endif


