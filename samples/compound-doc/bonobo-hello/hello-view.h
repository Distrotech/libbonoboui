#ifndef _INC_HELLO_BONOBO_VIEW_H
#define _INC_HELLO_BONOBO_VIEW_H

#include <bonobo.h>

#define HELLO_BONOBO_TYPE_VIEW        (hello_bonobo_view_get_type ())
#define HELLO_BONOBO_VIEW(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), HELLO_BONOBO_TYPE_VIEW, HelloBonoboView))
#define HELLO_BONOBO_VIEW_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), HELLO_BONOBO_TYPE_VIEW, HelloBonoboViewClass))
#define HELLO_BONOBO_IS_VIEW(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), HELLO_BONOBO_TYPE_VIEW))
#define HELLO_BONOBO_IS_VIEW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), HELLO_BONOBO_TYPE_VIEW))

struct _HelloBonoboView {
	BonoboView view;

	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *button;
};

typedef struct {
	BonoboViewClass parent_class;
} HelloBonoboViewClass;

GType     hello_bonobo_view_get_type (void);
BonoboView *hello_bonobo_view_factory  (BonoboEmbeddable      *embeddable,
					const Bonobo_ViewFrame view_frame,
					void                  *closure);
void        hello_view_update          (HelloBonoboView       *view,
					HelloBonoboEmbeddable *embeddable);

#endif /* _INC_HELLO_BONOBO_EMBEDDABLE_H */
