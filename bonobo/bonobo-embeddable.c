/**
 * GNOME BoboboObject object.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-component.h>

static GnomeObjectClass *gnome_bonobo_object_parent_class;

enum {
	HOST_NAME_CHANGED,
	DO_VERB,
	LAST_SIGNAL
};

static guint bonobo_object_signals [LAST_SIGNAL];

static void
impl_GNOME_BonoboObject_do_verb (PortableServer_Servant servant,
				 const CORBA_char *verb_name,
				 CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

	gtk_signal_emit (
		GTK_OBJECT (bonobo_object),
		bonobo_object_signals [DO_VERB],
		(gchar *) verb_name);
}

static void
impl_GNOME_BonoboObject_set_client_site (PortableServer_Servant servant,
					 const GNOME_ClientSite client_site,
					 CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

	bonobo_object->client_site = client_site;
}

static GNOME_ClientSite
impl_GNOME_BonoboObject_get_client_site (PortableServer_Servant servant,
					 CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

	return bonobo_object->client_site;
}

static void
impl_GNOME_BonoboObject_set_host_name (PortableServer_Servant servant,
				       const CORBA_char *name,
				       const CORBA_char *appname,
				       CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

	if (bonobo_object->host_name)
		g_free (bonobo_object->host_name);
	if (bonobo_object->host_appname)
		g_free (bonobo_object->host_appname);

	bonobo_object->host_name = g_strdup (name);
	bonobo_object->host_appname = g_strdup (appname);

	gtk_signal_emit (GTK_OBJECT (bonobo_object),
			 bonobo_object_signals [HOST_NAME_CHANGED]);
}


static void
impl_GNOME_BonoboObject_close (PortableServer_Servant servant,
			       const GNOME_BonoboObject_CloseMode mode,
			       CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));
}

static void
impl_GNOME_BonoboObject_set_moniker (PortableServer_Servant servant,
				     const GNOME_Moniker mon,
				     const GNOME_Moniker_type which,
				     CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

}

static GNOME_BonoboObject_verb_list *
impl_GNOME_BonoboObject_get_verb_list (PortableServer_Servant servant,
				       CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));
	GNOME_BonoboObject_verb_list *list;
	GList *l;
	int len, i;

	len = g_list_length (bonobo_object->verbs);

	if (len == 0)
		return NULL;
	
	list = GNOME_BonoboObject_verb_list__alloc ();
	
	list->_length = len;
	list->_maximum = len;
	list->_buffer = CORBA_sequence_CORBA_string_allocbuf (len);

	if (list->_buffer == NULL){
		CORBA_free (list);
		return NULL;
	}

	for (i = 0, l = bonobo_object->verbs; l; l = l->next, i++)
		list->_buffer [i] = CORBA_string_dup (l->data);
	
	return list;
}

static void
impl_GNOME_BonoboObject_advise (PortableServer_Servant servant,
				const GNOME_AdviseSink advise,
				CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));
}

static void
impl_GNOME_BonoboObject_unadvise (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));
}

static CORBA_long
impl_GNOME_BonoboObject_get_misc_status (PortableServer_Servant servant,
					 const CORBA_long type,
					 CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));

	return 0;
}

static GNOME_View
impl_GNOME_BonoboObject_new_view (PortableServer_Servant servant, CORBA_Environment *ev)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (gnome_object_from_servant (servant));
	GnomeView *view;
	
	view = bonobo_object->view_factory (bonobo_object, bonobo_object->view_factory_closure);

	if (view == NULL)
		return CORBA_OBJECT_NIL;
	
	return GNOME_OBJECT (view)->object;
}

POA_GNOME_BonoboObject__epv gnome_bonobo_object_epv = {
	NULL,
	&impl_GNOME_BonoboObject_do_verb,
	&impl_GNOME_BonoboObject_set_client_site,
	&impl_GNOME_BonoboObject_get_client_site,
	&impl_GNOME_BonoboObject_set_host_name,
	&impl_GNOME_BonoboObject_close,
	&impl_GNOME_BonoboObject_set_moniker,
	&impl_GNOME_BonoboObject_get_verb_list,
	&impl_GNOME_BonoboObject_advise,
	&impl_GNOME_BonoboObject_unadvise,
	&impl_GNOME_BonoboObject_get_misc_status,
	&impl_GNOME_BonoboObject_new_view
};

static POA_GNOME_BonoboObject__vepv gnome_bonobo_object_vepv = {
	&gnome_obj_base_epv,
	&gnome_obj_epv,
	&gnome_bonobo_object_epv
};

static CORBA_Object
create_gnome_bonobo_object (GnomeObject *object)
{
	POA_GNOME_BonoboObject *servant;
	CORBA_Object o;
	
	servant = g_new0 (POA_GNOME_BonoboObject, 1);
	servant->vepv = &gnome_bonobo_object_vepv;

	POA_GNOME_BonoboObject__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_object_activate_servant (object, servant);
}

GnomeBonoboObject *
gnome_bonobo_object_construct (GnomeBonoboObject  *bonobo_object,
			       GNOME_BonoboObject  corba_bonobo_object,
			       GnomeViewFactory factory,
			       void *data)
{
	g_return_val_if_fail (bonobo_object != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_BONOBO_OBJECT (bonobo_object), NULL);
	g_return_val_if_fail (factory != NULL, NULL);
	g_return_val_if_fail (corba_bonobo_object != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (bonobo_object), corba_bonobo_object);

	bonobo_object->view_factory = factory;
	bonobo_object->view_factory_closure = data;
	
	return bonobo_object;
}

/**
 * gnome_bonobo_object_new:
 * @factory: Factory routine that provides new views of the bonobo_object on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a GNOME::BonoboObject CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the bonobo_object (bonobo_object should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a GnomeBonoboObject that contains an activated GNOME::BonoboObject
 * CORBA server.
 */
GnomeBonoboObject *
gnome_bonobo_object_new (GnomeViewFactory factory, void *data)
{
	GNOME_BonoboObject corba_bonobo_object;
	GnomeBonoboObject *bonobo_object;

	g_return_val_if_fail (factory != NULL, NULL);

	bonobo_object = gtk_type_new (gnome_bonobo_object_get_type ());
	corba_bonobo_object = create_gnome_bonobo_object (GNOME_OBJECT (bonobo_object));
	if (corba_bonobo_object == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (bonobo_object));
		return NULL;
	}
	
	return gnome_bonobo_object_construct (bonobo_object, corba_bonobo_object, factory, data);
}

static void
gnome_bonobo_object_destroy (GtkObject *object)
{
	GnomeBonoboObject *bonobo_object = GNOME_BONOBO_OBJECT (object);
	GList *l;

	/*
	 * Release the verbs
	 */
	for (l = bonobo_object->verbs; l; l = l->next)
		g_free (l->data);

	g_list_free (bonobo_object->verbs);
	
	GTK_OBJECT_CLASS (gnome_bonobo_object_parent_class)->destroy (object);
}

static void
gnome_bonobo_object_class_init (GnomeBonoboObjectClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_bonobo_object_parent_class =
		gtk_type_class (gnome_object_get_type ());

	bonobo_object_signals [DO_VERB] =
                gtk_signal_new ("do_verb",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeBonoboObjectClass, do_verb), 
                                gtk_marshal_NONE__INT_POINTER,
                                GTK_TYPE_NONE, 2,
                                GTK_TYPE_INT,
				GTK_TYPE_STRING);

	bonobo_object_signals [HOST_NAME_CHANGED] =
                gtk_signal_new ("host_name_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeBonoboObjectClass,host_name_changed), 
                                gtk_marshal_NONE__NONE,
                                GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, bonobo_object_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_bonobo_object_destroy;
}

static void
gnome_bonobo_object_init (GnomeObject *object)
{
}

GtkType
gnome_bonobo_object_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/BonoboObject:1.0",
			sizeof (GnomeBonoboObject),
			sizeof (GnomeBonoboObjectClass),
			(GtkClassInitFunc) gnome_bonobo_object_class_init,
			(GtkObjectInitFunc) gnome_bonobo_object_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

void
gnome_bonobo_object_set_view_factory (GnomeBonoboObject *bonobo_object,
				      GnomeViewFactory factory,
				      void *data)
{
	g_return_if_fail (bonobo_object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_OBJECT (bonobo_object));
	g_return_if_fail (factory != NULL);

	bonobo_object->view_factory = factory;
}

void
gnome_bonobo_object_add_verb (GnomeBonoboObject *bonobo_object, const char *verb_name)
{
	g_return_if_fail (bonobo_object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_OBJECT (bonobo_object));
	g_return_if_fail (verb_name != NULL);

	bonobo_object->verbs = g_list_prepend (bonobo_object->verbs, g_strdup (verb_name));
}

void
gnome_bonobo_object_add_verbs (GnomeBonoboObject *bonobo_object, const char **verbs)
{
	int i;
	
	g_return_if_fail (bonobo_object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_OBJECT (bonobo_object));
	g_return_if_fail (verbs != NULL);

	for (i = 0; verbs [i] != NULL; i++){
		bonobo_object->verbs = g_list_prepend (bonobo_object->verbs, g_strdup (verbs [i]));
	}
}

void
gnome_bonobo_object_remove_verb (GnomeBonoboObject *bonobo_object, const char *verb_name)
{
	GList *l;
	
	g_return_if_fail (bonobo_object != NULL);
	g_return_if_fail (GNOME_IS_BONOBO_OBJECT (bonobo_object));
	g_return_if_fail (verb_name != NULL);

	for (l = bonobo_object->verbs; l; l = l->data){
		if (strcmp (verb_name, l->data))
			continue;

		bonobo_object->verbs = g_list_remove (bonobo_object->verbs, l->data);
		g_free (l->data);
		return;
	}
}

