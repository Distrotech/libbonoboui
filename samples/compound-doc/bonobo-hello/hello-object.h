#ifndef HELLO_OBJECT_H
#define HELLO_OBJECT_H

#include <bonobo.h>
#include "hello-model.h"

BonoboEmbeddableFactory *factory;

BonoboObject *hello_object_factory (BonoboEmbeddableFactory * this,
				    void *data);

#endif
