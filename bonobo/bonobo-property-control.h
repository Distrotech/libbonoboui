/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-property-control.h: Property control implementation.
 *
 * Author:
 *      Iain Holmes  <iain@helixcode.com>
 *
 * Copyright 2000, Helix Code, Inc.
 */
#ifndef _BONOBO_PROPERTY_CONTROL_H_
#define _BONOBO_PROPERTY_CONTROL_H_

#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-control.h>

BEGIN_GNOME_DECLS

typedef struct _BonoboPropertyControl BonoboPropertyControl;
typedef struct _BonoboPropertyControlPrivate BonoboPropertyControlPrivate;

#define BONOBO_PROPERTY_CONTROL_TYPE        (bonobo_property_control_get_type ())
#define BONOBO_PROPERTY_CONTROL(o)          (GTK_CHECK_CAST ((o), BONOBO_PROPERTY_CONTROL_TYPE, BonoboPropertyControl))
#define BONOBO_PROPERTY_CONTROL_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_PROPERTY_CONTROL_TYPE, BonoboPropertyControlClass))
#define BONOBO_IS_PROPERTY_CONTROL(o)       (GTK_CHECK_TYPE ((o), BONOBO_PROPERTY_CONTROL_TYPE))
#define BONOBO_IS_PROPERTY_CONTROL_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_PROPERTY_CONTROL_TYPE))

typedef BonoboControl *(* BonoboPropertyControlGetControlFn) (BonoboPropertyControl *control,
							      void *closure);

struct _BonoboPropertyControl {
        BonoboObject		object;

	BonoboPropertyControlPrivate *priv;
};

typedef struct {
	BonoboObjectClass parent_class;

	void (* action) (BonoboPropertyControl *property_control, 
			 Bonobo_PropertyControl_Action action);
} BonoboPropertyControlClass;

GtkType bonobo_property_control_get_type (void);

POA_Bonobo_PropertyControl__epv *bonobo_property_control_get_epv (void);
Bonobo_PropertyControl bonobo_property_control_corba_object_create (BonoboObject *object);

BonoboPropertyControl *bonobo_property_control_construct (BonoboPropertyControl *property_control,
							  Bonobo_PropertyControl corba_control,
							  BonoboPropertyControlGetControlFn get_fn,
							  void *closure);
BonoboPropertyControl *bonobo_property_control_new (BonoboPropertyControlGetControlFn get_fn,
						    void *closure);


END_GNOME_DECLS

#endif /* _BONOBO_PROPERTY_CONTROL_H_ */

