/**
 * GNOME component object
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-component.h>

static GnomeObjectClass *gnome_component_parent_class;

enum {
	HOST_NAME_CHANGED,
	DO_VERB,
	LAST_SIGNAL
};

static guint component_signals [LAST_SIGNAL];

static void
impl_GNOME_Component_do_verb (PortableServer_Servant servant,
			      const CORBA_short verb,
			      const CORBA_char *verb_name,
			      CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

	gtk_signal_emit (
		component,
		component_signals [DO_VERB],
		verb, verb_name);
}

static void
impl_GNOME_Component_set_client_site (PortableServer_Servant servant,
				      const GNOME_ClientSite client_site,
				      CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

	component->client_site = client_site;
}

static GNOME_ClientSite
impl_GNOME_Component_get_client_site (PortableServer_Servant servant,
				      CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

	return component->client_site;
}

static void
impl_GNOME_Component_set_host_name (PortableServer_Servant servant,
				    const CORBA_char *name,
				    const CORBA_char *appname,
				    CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

	if (component->host_name)
		g_free (component->host_name);
	if (component->host_appname)
		g_free (component->host_appname);

	component->host_name = g_strdup (name);
	component->host_appname = g_strdup (appname);

	gtk_signal_emit (GTK_OBJECT (component),
			 component_signals [HOST_NAME_CHANGED]);
}


static void
impl_GNOME_Component_close (PortableServer_Servant servant,
			    const GNOME_Component_CloseMode mode,
			    CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));
}

static void
impl_GNOME_Component_set_moniker (PortableServer_Servant servant,
				  const GNOME_Moniker mon,
				  const GNOME_Moniker_type which,
				  CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

}

static GNOME_Component_verb_list *
impl_GNOME_Component_get_verb_list (PortableServer_Servant servant,
				    CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));
	GNOME_Component_verb_list *list;

	list = g_new (GNOME_Component_verb_list, 1);

	list->_length = 0;
	list->_maximum = 0;
	list->_buffer = 0;
	
	return list;
}

static void
impl_GNOME_Component_advise (PortableServer_Servant servant,
			     const GNOME_AdviseSink advise,
			     CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));
}

static void
impl_GNOME_Component_unadvise (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));
}

static CORBA_long
impl_GNOME_Component_get_misc_status (PortableServer_Servant servant,
				      const CORBA_long type,
				      CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));

	return 0;
}

static GNOME_View
impl_GNOME_Component_new_view (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeComponent *component = GNOME_COMPONENT (gnome_object_from_servant (servant));
	GnomeView *view;
	
	view = component->view_factory (component, component->view_factory_closure);

	if (view == NULL)
		return CORBA_OBJECT_NIL;
	
	return GNOME_OBJECT (view)->object;
}

POA_GNOME_Component__epv gnome_component_epv = {
	NULL,
	&impl_GNOME_Component_do_verb,
	&impl_GNOME_Component_set_client_site,
	&impl_GNOME_Component_get_client_site,
	&impl_GNOME_Component_set_host_name,
	&impl_GNOME_Component_close,
	&impl_GNOME_Component_set_moniker,
	&impl_GNOME_Component_get_verb_list,
	&impl_GNOME_Component_advise,
	&impl_GNOME_Component_unadvise,
	&impl_GNOME_Component_get_misc_status,
	&impl_GNOME_Component_new_view
};

static POA_GNOME_Component__vepv gnome_component_vepv = {
	&gnome_object_base_epv,
	&gnome_object_epv,
	&gnome_component_epv
};

static CORBA_Object
create_gnome_component (GnomeObject *object)
{
	POA_GNOME_Component *servant;
	CORBA_Object o;
	
	servant = g_new0 (POA_GNOME_Component, 1);
	servant->vepv = &gnome_component_vepv;

	POA_GNOME_Component__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_object_activate_servant (object, servant);
}

GnomeComponent *
gnome_component_construct (GnomeComponent  *component,
			   GNOME_Component  corba_component,
			   GnomeViewFactory factory,
			   void *data)
{
	g_return_val_if_fail (component != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_COMPONENT (component), NULL);
	g_return_val_if_fail (factory != NULL, NULL);
	g_return_val_if_fail (corba_component != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (component), corba_component);

	component->view_factory = factory;
	component->view_factory_closure = data;
	
	return component;
}

/**
 * gnome_component_new:
 * @factory: Factory routine that provides new views of the component on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a GNOME::Component CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the component (component should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a GnomeComponent that contains an activated GNOME::Component
 * CORBA server.
 */
GnomeComponent *
gnome_component_new (GnomeViewFactory factory, void *data)
{
	GNOME_Component corba_component;
	GnomeComponent *component;

	g_return_val_if_fail (factory != NULL, NULL);

	component = gtk_type_new (gnome_component_get_type ());
	corba_component = create_gnome_component (GNOME_OBJECT (component));
	if (corba_component == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (component));
		return NULL;
	}
	
	return gnome_component_construct (component, corba_component, factory, data);
}

static void
gnome_component_destroy (GtkObject *object)
{
	GnomeComponent *component = GNOME_COMPONENT (object);

	GTK_OBJECT_CLASS (gnome_component_parent_class)->destroy (object);
}

static void
gnome_component_class_init (GnomeComponentClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_component_parent_class = gtk_type_class (gtk_object_get_type ());

	component_signals [DO_VERB] =
                gtk_signal_new ("do_verb",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeComponentClass, do_verb), 
                                gtk_marshal_NONE__INT_POINTER,
                                GTK_TYPE_NONE, 2,
                                GTK_TYPE_INT,
				GTK_TYPE_STRING); 

	component_signals [HOST_NAME_CHANGED] =
                gtk_signal_new ("host_name_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeComponentClass,host_name_changed), 
                                gtk_marshal_NONE__NONE,
                                GTK_TYPE_NONE, 0);
	
	object_class->destroy = gnome_component_destroy;
}

static void
gnome_component_init (GnomeObject *object)
{
}

GtkType
gnome_component_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/Component:1.0",
			sizeof (GnomeComponent),
			sizeof (GnomeComponentClass),
			(GtkClassInitFunc) gnome_component_class_init,
			(GtkObjectInitFunc) gnome_component_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

void
gnome_component_set_view_factory (GnomeComponent *component,
				  GnomeViewFactory factory,
				  void *data)
{
	g_return_if_fail (component != NULL);
	g_return_if_fail (GNOME_IS_COMPONENT (component));
	g_return_if_fail (factory != NULL);

	component->view_factory = factory;
}


