/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Embeddable object.
 *
 * A BonoboEmbeddable object represents the actual object being
 * embedded.  A BonoboEmbeddable may have one or more BonoboViews, each
 * of which is an identical embedded window which displays the
 * BonoboEmbeddable's contents.  The BonoboEmbeddable is associated with
 * a BonoboClientSite, which is a container-side object with which the
 * BonoboEmbeddable communicates.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-embeddable.h>

static BonoboObjectClass *bonobo_embeddable_parent_class;

enum {
	HOST_NAME_CHANGED,
	URI_CHANGED,
	LAST_SIGNAL
};

static guint embeddable_signals [LAST_SIGNAL];

POA_Bonobo_Embeddable__vepv bonobo_embeddable_vepv;

struct _BonoboEmbeddablePrivate {
	/*
	 * The instantiated views for this Embeddable.
	 */
	GList *views;

	/*
	 * The instantiated Canvas Items for this Embeddable.
	 */
	GList *canvas_items;
	
	/*
	 * The View factory
	 */
	BonoboViewFactory view_factory;
	void *view_factory_closure;

	/*
	 * For the Canvas Item
	 */
	GnomeItemCreator item_creator;
	void *item_creator_data;
};

static void
impl_Bonobo_Embeddable_set_client_site (PortableServer_Servant servant,
				       const Bonobo_ClientSite client_site,
				       CORBA_Environment *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));
	CORBA_Environment evx;

	CORBA_exception_init (&evx);

	if (embeddable->client_site != CORBA_OBJECT_NIL)
		CORBA_Object_release (client_site, &evx);
	
	embeddable->client_site = CORBA_Object_duplicate (client_site, &evx);
        CORBA_exception_free (&evx);							     
}

static Bonobo_ClientSite
impl_Bonobo_Embeddable_get_client_site (PortableServer_Servant servant,
				       CORBA_Environment *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));
	Bonobo_ClientSite ret;
	CORBA_Environment evx;
	
	CORBA_exception_init (&evx);
	ret = CORBA_Object_duplicate (embeddable->client_site, &evx);
        CORBA_exception_free (&evx);							     

	return ret;
}

static void
impl_Bonobo_Embeddable_set_host_name (PortableServer_Servant servant,
				     const CORBA_char      *name,
				     const CORBA_char      *appname,
				     CORBA_Environment     *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));

	if (embeddable->host_name)
		g_free (embeddable->host_name);
	if (embeddable->host_appname)
		g_free (embeddable->host_appname);

	embeddable->host_name    = g_strdup (name);
	embeddable->host_appname = g_strdup (appname);

	gtk_signal_emit (GTK_OBJECT (embeddable),
			 embeddable_signals [HOST_NAME_CHANGED]);
}


static void
impl_Bonobo_Embeddable_close (PortableServer_Servant servant,
			     const Bonobo_Embeddable_CloseMode mode,
			     CORBA_Environment *ev)
{
}

static Bonobo_Embeddable_verb_list *
impl_Bonobo_Embeddable_get_verb_list (PortableServer_Servant servant,
				     CORBA_Environment *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));
	Bonobo_Embeddable_verb_list *verb_list;

	GList *l;
	int len;
	int i;

	len = g_list_length (embeddable->verbs);

	verb_list = Bonobo_Embeddable_verb_list__alloc ();
	verb_list->_length = len;

	if (len == 0)
		return verb_list;

	verb_list->_buffer = CORBA_sequence_Bonobo_Embeddable_GnomeVerb_allocbuf (len);

	for (i = 0, l = embeddable->verbs; l != NULL; l = l->next, i ++) {
		Bonobo_Embeddable_GnomeVerb *corba_verb;
		GnomeVerb *verb = (GnomeVerb *) l->data;

		corba_verb = & verb_list->_buffer [i];
#define CORBIFY_STRING(s) ((s) == NULL ? "" : (s))
		corba_verb->name  = CORBA_string_dup (CORBIFY_STRING (verb->name));
		corba_verb->label = CORBA_string_dup (CORBIFY_STRING (verb->label));
		corba_verb->hint  = CORBA_string_dup (CORBIFY_STRING (verb->hint));
	}

	return verb_list;
}

static void
impl_Bonobo_Embeddable_advise (PortableServer_Servant servant,
			      const Bonobo_AdviseSink advise,
			      CORBA_Environment *ev)
{
}

static void
impl_Bonobo_Embeddable_unadvise (PortableServer_Servant servant, CORBA_Environment *ev)
{
}

static CORBA_long
impl_Bonobo_Embeddable_get_misc_status (PortableServer_Servant servant,
				       const CORBA_long type,
				       CORBA_Environment *ev)
{

	return 0;
}

static void
ping_container (BonoboEmbeddable *embeddable)
{
	/*
	 * If all of the views are gone, that *might* mean that
	 * our container application died.  So ping it to find
	 * out if it's still alive.
	 */
	if ((embeddable->priv->views != NULL) || (embeddable->priv->canvas_items != NULL))
		return;

	/*
	 * ORBit has some issues.
	 *
	 * Calling gnome_unknown_ping on a dead object causes either:
	 * 
	 *     . SIGPIPE (handled; please see gnome-main.c)
	 *     . A blocking call to connect().
	 *
	 * So we just assume the remote end is dead here.  This ORBit
	 * problem needs to be fixed, though, so that this code can be
	 * re-enabled.
	 */
	if (0) {
/*	if (! gnome_unknown_ping (embeddable->client_site)) {*/
		/*
		 * The remote end is dead; it's time for
		 * us to die too.
		 */
		bonobo_object_destroy (BONOBO_OBJECT (embeddable));
	}
}

static void
bonobo_embeddable_view_destroy_cb (BonoboView *view, gpointer data)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (data);

	/*
	 * Remove this view from our list of views.
	 */
	embeddable->priv->views = g_list_remove (embeddable->priv->views, view);

	ping_container (embeddable);
}

static Bonobo_View
impl_Bonobo_Embeddable_new_view (PortableServer_Servant servant,
				Bonobo_ViewFrame view_frame,
				CORBA_Environment *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));
	BonoboView *view;
	CORBA_Environment evx;
	Bonobo_View ret;
	
	view = embeddable->priv->view_factory (
		embeddable, view_frame,
		embeddable->priv->view_factory_closure);

	if (view == NULL)
		return CORBA_OBJECT_NIL;

	if (bonobo_object_corba_objref (BONOBO_OBJECT (view)) == CORBA_OBJECT_NIL){
		g_warning ("Returned view does not have a CORBA object bound\n");
		gtk_object_destroy (GTK_OBJECT (view));
		return CORBA_OBJECT_NIL;
	}
	bonobo_view_set_view_frame (view, view_frame);
	bonobo_view_set_embeddable (view, embeddable);

	embeddable->priv->views = g_list_prepend (embeddable->priv->views, view);

	gtk_signal_connect (GTK_OBJECT (view), "destroy",
			    GTK_SIGNAL_FUNC (bonobo_embeddable_view_destroy_cb), embeddable);

	CORBA_exception_init (&evx);
	ret = CORBA_Object_duplicate (bonobo_object_corba_objref (BONOBO_OBJECT (view)), &evx);
	CORBA_exception_free (&evx);

	return ret;
}

static void
impl_Bonobo_Embeddable_set_uri (PortableServer_Servant servant,
			       const CORBA_char      *uri,
			       CORBA_Environment     *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));

	bonobo_embeddable_set_uri (embeddable, uri);
}

static void
canvas_item_destroyed (BonoboCanvasComponent *comp, BonoboEmbeddable *embeddable)
{
	GnomeCanvasItem *item;

	item = bonobo_canvas_component_get_item (comp);
	gtk_object_destroy (GTK_OBJECT (item->canvas));
	
	/*
	 * Remove the canvas item from the list of items we keep
	 */
	embeddable->priv->canvas_items = g_list_remove (embeddable->priv->canvas_items, comp);

	ping_container (embeddable);
}

static BonoboCanvasComponent *
make_canvas_component (BonoboEmbeddable *embeddable, gboolean aa, Bonobo_Canvas_ComponentProxy item_proxy)
{
	BonoboCanvasComponent *component;
	GnomeCanvas *pseudo_canvas;
	
	if (aa){
		gdk_rgb_init ();
		pseudo_canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
	} else
		pseudo_canvas = GNOME_CANVAS (gnome_canvas_new ());
	
	component = (*embeddable->priv->item_creator)(
		embeddable, pseudo_canvas,
		embeddable->priv->item_creator_data);

	if (component == NULL){
		gtk_object_destroy (GTK_OBJECT (pseudo_canvas));
		return NULL;
	}
	bonobo_canvas_component_set_proxy (component, item_proxy);

	/*
	 * Now keep track of it
	 */
	embeddable->priv->canvas_items = g_list_prepend (embeddable->priv->canvas_items, component);
	gtk_signal_connect (GTK_OBJECT (component), "destroy",
			    GTK_SIGNAL_FUNC (canvas_item_destroyed), embeddable);
	
	return component;
}

static Bonobo_Canvas_Component
impl_Bonobo_Embeddable_new_canvas_item (PortableServer_Servant servant,
				       CORBA_boolean aa,
				       Bonobo_Canvas_ComponentProxy _item_proxy,
				       CORBA_Environment *ev)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (bonobo_object_from_servant (servant));
	Bonobo_Canvas_ComponentProxy item_proxy;
	BonoboCanvasComponent *component;
	
	if (embeddable->priv->item_creator == NULL)
		return CORBA_OBJECT_NIL;

	item_proxy = CORBA_Object_duplicate (_item_proxy, ev);
	
	component = make_canvas_component (embeddable, aa, item_proxy);

	return CORBA_Object_duplicate (bonobo_object_corba_objref (BONOBO_OBJECT (component)), ev);
}

/**
 * bonobo_embeddable_get_epv:
 */
POA_Bonobo_Embeddable__epv *
bonobo_embeddable_get_epv (void)
{
	POA_Bonobo_Embeddable__epv *epv;

	epv = g_new0 (POA_Bonobo_Embeddable__epv, 1);

	epv->set_client_site = impl_Bonobo_Embeddable_set_client_site;
	epv->get_client_site = impl_Bonobo_Embeddable_get_client_site;
	epv->set_host_name   = impl_Bonobo_Embeddable_set_host_name;
	epv->close           = impl_Bonobo_Embeddable_close;
	epv->get_verb_list   = impl_Bonobo_Embeddable_get_verb_list;
	epv->advise          = impl_Bonobo_Embeddable_advise;
	epv->unadvise        = impl_Bonobo_Embeddable_unadvise;
	epv->get_misc_status = impl_Bonobo_Embeddable_get_misc_status;
	epv->new_view        = impl_Bonobo_Embeddable_new_view;
	epv->set_uri         = impl_Bonobo_Embeddable_set_uri;
	epv->new_canvas_item = impl_Bonobo_Embeddable_new_canvas_item;

	return epv;
}

static void
bonobo_embeddable_corba_class_init ()
{
	bonobo_embeddable_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_embeddable_vepv.Bonobo_Embeddable_epv = bonobo_embeddable_get_epv ();
}

/**
 * bonobo_embeddable_corba_object_create:
 * @object: The GtkObject that will wrap the CORBA object.
 *
 * Creates an activates the CORBA object that is wrapped
 * by the BonoboObject @object.
 *
 * Returns: An activated object reference to the created object or
 * %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_Embeddable
bonobo_embeddable_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_Embeddable *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_Embeddable *)g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_embeddable_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_Embeddable__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);

	return bonobo_object_activate_servant (object, servant);
}

/**
 * bonobo_embeddable_construct:
 * @embeddable: BonoboEmbeddable object to construct.
 * @corba_embeddable: The CORBA reference that implements this object.
 * @view_factory: Factory routine that provides new views of the embeddable on demand
 * @view_factory_data: pointer passed to the @view_factory routine to provide context.
 * @item_factory: A factory routine that creates BonoboCanvasComponents.
 * @item_factory_data: pointer passed to the @item_factory routine.
 *
 * This routine constructs a Bonobo::Embeddable CORBA server and activates it.
 *
 * The @view_factory routine will be invoked by this CORBA server when
 * a request arrives to get a new view of the embeddable (embeddable
 * should be able to provide multiple views of themselves upon demand).
 * The @view_factory_data pointer is passed to this factory routine untouched to
 * allow the factory to get some context on what it should create.
 *
 * The @item_factory will be invoked if the container application requests a
 * canvas item version of this embeddable.  The routine @item_factory will
 * be invoked with the @item_factory_data argument
 *
 * Returns: The constructed object.
 */
BonoboEmbeddable *
bonobo_embeddable_construct_full (BonoboEmbeddable *embeddable,
				 Bonobo_Embeddable corba_embeddable,
				 BonoboViewFactory view_factory,
				 void             *factory_data,
				 GnomeItemCreator item_factory,
				 void             *item_factory_data)
{
	
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_EMBEDDABLE (embeddable), NULL);
	g_return_val_if_fail (corba_embeddable != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (embeddable), corba_embeddable);

	embeddable->priv->view_factory         = view_factory;
	embeddable->priv->view_factory_closure = factory_data;
	embeddable->priv->item_creator         = item_factory;
	embeddable->priv->item_creator_data    = item_factory_data;
		
	return embeddable;
}

/**
 * bonobo_embeddable_construct:
 * @embeddable: BonoboEmbeddable object to construct.
 * @corba_embeddable: The CORBA reference that implements this object.
 * @factory: Factory routine that provides new views of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 * 
 * This routine constructs a Bonobo::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns: The constructed object.
 */
BonoboEmbeddable *
bonobo_embeddable_construct (BonoboEmbeddable  *embeddable,
			    Bonobo_Embeddable   corba_embeddable,
			    BonoboViewFactory   factory,
			    void               *data)
{
	return bonobo_embeddable_construct_full (embeddable, corba_embeddable, factory, data, NULL, NULL);
}

/**
 * bonobo_embeddable_new:
 * @factory: Factory routine that provides new views of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a Bonobo::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a BonoboEmbeddable that contains an activated Bonobo::Embeddable
 * CORBA server.
 */
BonoboEmbeddable *
bonobo_embeddable_new (BonoboViewFactory factory, void *data)
{
	Bonobo_Embeddable corba_embeddable;
	BonoboEmbeddable *embeddable;

	g_return_val_if_fail (factory != NULL, NULL);

	embeddable = gtk_type_new (BONOBO_EMBEDDABLE_TYPE);

	corba_embeddable = bonobo_embeddable_corba_object_create (BONOBO_OBJECT (embeddable));
	if (corba_embeddable == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (embeddable));
		return NULL;
	}
	
	return bonobo_embeddable_construct (embeddable, corba_embeddable, factory, data);
}

/**
 * bonobo_embeddable_new_canvas_item:
 * @item_factory: Factory routine that provides new canvas items of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a Bonobo::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a BonoboEmbeddable that contains an activated Bonobo::Embeddable
 * CORBA server.
 */
BonoboEmbeddable *
bonobo_embeddable_new_canvas_item (GnomeItemCreator item_factory, void *data)
{
	Bonobo_Embeddable corba_embeddable;
	BonoboEmbeddable *embeddable;

	g_return_val_if_fail (item_factory != NULL, NULL);

	embeddable = gtk_type_new (BONOBO_EMBEDDABLE_TYPE);

	corba_embeddable = bonobo_embeddable_corba_object_create (BONOBO_OBJECT (embeddable));
	if (corba_embeddable == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (embeddable));
		return NULL;
	}
	
	return bonobo_embeddable_construct_full (embeddable, corba_embeddable, NULL, NULL, item_factory, data);
}

static void
bonobo_embeddable_destroy (GtkObject *object)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (object);
	GList *l;

	/*
	 * Destroy all our views.
	 */
	while (embeddable->priv->views) {
		BonoboView *view = BONOBO_VIEW (embeddable->priv->views->data);

		bonobo_object_destroy (BONOBO_OBJECT (view));
	}

	while (embeddable->priv->canvas_items){
		void *data = embeddable->priv->canvas_items->data;
		BonoboCanvasComponent *comp = BONOBO_CANVAS_COMPONENT (data);

		bonobo_object_destroy (BONOBO_OBJECT (comp));
	}
	
	/*
	 * Release the verbs
	 */
	for (l = embeddable->verbs; l; l = l->next) {
		GnomeVerb *verb = (GnomeVerb *) l->data;

		g_free (verb->name);
		g_free (verb->label);
		g_free (verb->hint);
		g_free (verb);
	}
	g_list_free (embeddable->verbs);

	if (embeddable->uri)
		g_free (embeddable->uri);
	
	/*
	 * Release any references we might keep
	 */
	if (embeddable->client_site != CORBA_OBJECT_NIL){
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		CORBA_Object_release (embeddable->client_site, &ev);
		CORBA_exception_free (&ev);
	}

	g_free (embeddable->priv);
	
	GTK_OBJECT_CLASS (bonobo_embeddable_parent_class)->destroy (object);
}

static void
bonobo_embeddable_class_init (BonoboEmbeddableClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_embeddable_parent_class =
		gtk_type_class (bonobo_object_get_type ());

	embeddable_signals [HOST_NAME_CHANGED] =
                gtk_signal_new ("host_name_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(BonoboEmbeddableClass,host_name_changed), 
                                gtk_marshal_NONE__STRING,
                                GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
	embeddable_signals [URI_CHANGED] =
                gtk_signal_new ("uri_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(BonoboEmbeddableClass,uri_changed), 
                                gtk_marshal_NONE__STRING,
                                GTK_TYPE_NONE, 1, GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, embeddable_signals,
				      LAST_SIGNAL);

	object_class->destroy = bonobo_embeddable_destroy;

	bonobo_embeddable_corba_class_init ();
}

static void
bonobo_embeddable_init (BonoboObject *object)
{
	BonoboEmbeddable *embeddable = BONOBO_EMBEDDABLE (object);

	embeddable->priv = g_new0 (BonoboEmbeddablePrivate, 1);
}

/**
 * bonobo_embeddable_get_type:
 *
 * Returns: The GtkType for the BonoboEmbeddable class.
 */
GtkType
bonobo_embeddable_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboEmbeddable",
			sizeof (BonoboEmbeddable),
			sizeof (BonoboEmbeddableClass),
			(GtkClassInitFunc) bonobo_embeddable_class_init,
			(GtkObjectInitFunc) bonobo_embeddable_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_embeddable_set_view_factory:
 * @embeddable: The embeddable object to operate on.
 * @factory: A pointer to a function that can provide BonoboView objects on demand.
 * @data: data to pass to the @factory function.
 *
 * This routine defines the view factory for this embeddable component.
 * When a container requires a view, the routine specified in @factory
 * will be invoked to create a new BonoboView object to satisfy this request.
 */
void
bonobo_embeddable_set_view_factory (BonoboEmbeddable *embeddable,
				   BonoboViewFactory factory,
				   void *data)
{
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (factory != NULL);

	embeddable->priv->view_factory = factory;
	embeddable->priv->view_factory_closure = data;
}

/**
 * bonobo_embeddable_add_verb:
 * @embeddable: The embeddable object to operate on.
 * @verb_name: The key which is used to uniquely identify the verb.
 * @verb_label: A localizable string which identifies the verb.
 * @verb_hint: A localizable string which gives a verbose description
 * of the verb's function.
 *
 * This routine adds @verb_name to the list of verbs supported
 * by this @embeddable.
 */
void
bonobo_embeddable_add_verb (BonoboEmbeddable *embeddable,
			   const char *verb_name, const char *verb_label, const char *verb_hint)
{
	GnomeVerb *verb;

	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (verb_name != NULL);

	verb = g_new0 (GnomeVerb, 1);
	verb->name = g_strdup (verb_name);
	verb->label = g_strdup (verb_label);
	verb->hint = g_strdup (verb_hint);

	embeddable->verbs = g_list_prepend (embeddable->verbs, verb);
}

/**
 * bonobo_embeddable_add_verbs:
 * @embeddable: The embeddable object to operate on.
 * @verbs: An array of GnomeVerbs to be added.
 *
 * This routine adds the list of verbs in the NULL terminated array
 * in @verbs to the exported verbs for the component.
 */
void
bonobo_embeddable_add_verbs (BonoboEmbeddable *embeddable, const GnomeVerb *verbs)
{
	int i;

	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (verbs != NULL);

	for (i = 0; verbs [i].name != NULL; i++)
		bonobo_embeddable_add_verb (embeddable, verbs[i].name, verbs[i].label, verbs[i].hint);
}

/**
 * bonobo_embeddable_remove_verb:
 * @embeddable: The embeddable object to operate on.
 * @verb_name: a verb name
 *
 * This routine removes the verb called @verb_name from the list
 * of exported verbs for this embeddable object
 */
void
bonobo_embeddable_remove_verb (BonoboEmbeddable *embeddable, const char *verb_name)
{
	GList *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (verb_name != NULL);

	for (l = embeddable->verbs; l != NULL; l = l->next) {
		GnomeVerb *verb = (GnomeVerb *) l->data;

		if (! strcmp (verb_name, verb->name)) {
			embeddable->verbs = g_list_remove_link (embeddable->verbs, l);
			g_list_free_1 (l);

			g_free (verb->name);
			g_free (verb->label);
			g_free (verb->hint);
			g_free (verb);

			return;
		}
	}

	g_warning ("Verb [%s] not found!\n", verb_name);
}

/**
 * bonobo_embeddable_get_verbs:
 * @embeddable: A BonoboEmbeddable object.
 *
 * Returns the internal copy of the list of verbs supported by this
 * Embeddable object.
 */
const GList *
bonobo_embeddable_get_verbs (BonoboEmbeddable *embeddable)
{
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_EMBEDDABLE (embeddable), NULL);

	return (const GList *) embeddable->verbs;
}


/**
 * bonobo_embeddable_get_uri:
 * @embeddable: The embeddable object to operate on.
 *
 * Returns the URI that this object represents
 */
const char *
bonobo_embeddable_get_uri (BonoboEmbeddable *embeddable)
{
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_EMBEDDABLE (embeddable), NULL);

	return embeddable->uri;
}

/**
 * bonobo_embeddable_set_uri:
 * @embeddable: The embeddable object to operate on.
 * @uri: the URI this embeddable represents.
 *
 * Sets the URI that this object represents.
 */
void
bonobo_embeddable_set_uri (BonoboEmbeddable *embeddable, const char *uri)
{
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));

	if (embeddable->uri){
		g_free (embeddable->uri);
		embeddable->uri = NULL;
	}
	
	if (uri)
		embeddable->uri = g_strdup (uri);

	gtk_signal_emit (GTK_OBJECT (embeddable),
			 embeddable_signals [URI_CHANGED],
			 embeddable->uri);
}

/**
 * bonobo_embeddable_foreach_view:
 * @embeddable: Embeddable on which we operate
 * @fn: function to be invoked for each existing BonoboView
 * @data: data to pass to function
 *
 * Invokes the @fn function for each view existing
 */
void
bonobo_embeddable_foreach_view (BonoboEmbeddable *embeddable,
			       BonoboEmbeddableForeachViewFn fn,
			       void *data)
{
	GList *copy, *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (fn != NULL);

	copy = g_list_copy (embeddable->priv->views);
	for (l = copy; l; l = l->next)
		(*fn)(BONOBO_VIEW (l->data), data);

	g_list_free (copy);
}

/**
 * bonobo_embeddable_foreach_item:
 * @embeddable: Embeddable on which we operate
 * @fn: function to be invoked for each existing GnomeItem
 * @data: data to pass to function
 *
 * Invokes the @fn function for each item existing
 */
void
bonobo_embeddable_foreach_item (BonoboEmbeddable *embeddable,
			       BonoboEmbeddableForeachItemFn fn,
			       void *data)
{
	GList *copy, *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (BONOBO_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (fn != NULL);

	copy = g_list_copy (embeddable->priv->canvas_items);
	for (l = copy; l; l = l->next)
		(*fn)(BONOBO_CANVAS_COMPONENT (l->data), data);

	g_list_free (copy);
}
