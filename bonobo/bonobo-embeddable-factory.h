/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME EmbeddableFactory object.
 *
 * Left for compatibility reasons, you should use BonoboGenericFactory instead.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#ifndef _GNOME_COMPONENT_FACTORY_H_
#define _GNOME_COMPONENT_FACTORY_H_

#include <glib/gmacros.h>
#include <gtk/gtkobject.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-view.h>
#include <bonobo/bonobo-embeddable.h>
#include <bonobo/bonobo-generic-factory.h>

G_BEGIN_DECLS
 
#define BONOBO_EMBEDDABLE_FACTORY_TYPE        BONOBO_GENERIC_FACTORY_TYPE
#define BONOBO_EMBEDDABLE_FACTORY(o)          BONOBO_GENERIC_FACTORY(o)
#define BONOBO_EMBEDDABLE_FACTORY_CLASS(k)    BONOBO_GENERIC_FACTORY_CLASS(k)
#define BONOBO_IS_EMBEDDABLE_FACTORY(o)       BONOBO_IS_GENERIC_FACTORY(o)
#define BONOBO_IS_EMBEDDABLE_FACTORY_CLASS(k) BONOBO_IS_GENERIC_FACTORY_CLASS(k)

typedef BonoboGenericFactory BonoboEmbeddableFactory;
typedef BonoboGenericFactoryClass BonoboEmbeddableFactoryClass;
typedef BonoboGenericFactoryFn BonoboEmbeddableFactoryFn;
					
#define bonobo_embeddable_factory_get_type  bonobo_generic_factory_get_type
#define bonobo_embeddable_factory_new       bonobo_generic_factory_new
#define bonobo_embeddable_factory_construct bonobo_generic_factory_construct
#define bonobo_embeddable_factory_set       bonobo_generic_factory_set

G_END_DECLS

#endif
