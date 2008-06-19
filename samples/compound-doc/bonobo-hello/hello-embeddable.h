#ifndef _INC_HELLO_BONOBO_EMBEDDABLE_H
#define _INC_HELLO_BONOBO_EMBEDDABLE_H

#include <bonobo.h>

typedef struct _HelloBonoboEmbeddable HelloBonoboEmbeddable;
typedef struct _HelloBonoboView       HelloBonoboView;

#define HELLO_BONOBO_TYPE_EMBEDDABLE        (hello_bonobo_embeddable_get_type ())
#define HELLO_BONOBO_EMBEDDABLE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), HELLO_BONOBO_TYPE_EMBEDDABLE, HelloBonoboEmbeddable))
#define HELLO_BONOBO_EMBEDDABLE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), HELLO_BONOBO_TYPE_EMBEDDABLE, HelloBonoboEmbeddableClass))
#define HELLO_BONOBO_IS_EMBEDDABLE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), HELLO_BONOBO_TYPE_EMBEDDABLE))
#define HELLO_BONOBO_IS_EMBEDDABLE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), HELLO_BONOBO_TYPE_EMBEDDABLE))

struct _HelloBonoboEmbeddable {
	BonoboEmbeddable embeddable;
	
	char            *text;
};

typedef struct {
	BonoboEmbeddableClass parent_class;
} HelloBonoboEmbeddableClass;

GType hello_bonobo_embeddable_get_type  (void);

HelloBonoboEmbeddable *
        hello_bonobo_embeddable_construct (HelloBonoboEmbeddable *embeddable);
void    hello_bonobo_embeddable_set_text  (HelloBonoboEmbeddable *embeddable,
					   char                  *text);

#endif /* _INC_HELLO_BONOBO_EMBEDDABLE_H */
