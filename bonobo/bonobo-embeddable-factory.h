/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME EmbeddableFactory object.
 *
 * Left for compatibility reasons, you should use GnomeGenericFactory instead.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#ifndef _GNOME_COMPONENT_FACTORY_H_
#define _GNOME_COMPONENT_FACTORY_H_

#include <libgnome/gnome-defs.h>
#include <gtk/gtkobject.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-view.h>
#include <bonobo/gnome-embeddable.h>

BEGIN_GNOME_DECLS
 
#define GNOME_EMBEDDABLE_FACTORY_TYPE        GNOME_GENERIC_FACTORY_TYPE
#define GNOME_EMBEDDABLE_FACTORY(o)          GNOME_GENERIC_FACTORY(o)
#define GNOME_EMBEDDABLE_FACTORY_CLASS(k)    GNOME_GENERIC_FACTORY_CLASS(k)
#define GNOME_IS_EMBEDDABLE_FACTORY(o)       GNOME_IS_GENERIC_FACTORY(o)
#define GNOME_IS_EMBEDDABLE_FACTORY_CLASS(k) GNOME_IS_GENERIC_FACTORY_CLASS(k)

typedef GnomeGenericFactory GnomeEmbeddableFactory;
typedef GnomeGenericFactoryClass GnomeEmbeddableFactoryClass;
typedef GnomeGenericFactoryFn GnomeEmbeddableFactoryFn;
					
#define gnome_embeddable_factory_get_type  gnome_generic_factory_get_type
#define gnome_embeddable_factory_new       gnome_generic_factory_new
#define gnome_embeddable_factory_construct gnome_generic_factory_construct
#define gnome_embeddable_factory_set       gnome_generic_factory_set (

END_GNOME_DECLS

#endif
