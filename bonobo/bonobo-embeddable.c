/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * GNOME Embeddable object.
 *
 * A GnomeEmbeddable object represents the actual object being
 * embedded.  A GnomeEmbeddable may have one or more GnomeViews, each
 * of which is an identical embedded window which displays the
 * GnomeEmbeddable's contents.  The GnomeEmbeddable is associated with
 * a GnomeClientSite, which is a container-side object with which the
 * GnomeEmbeddable communicates.
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Nat Friedman    (nat@nat.org)
 *
 * Copyright 1999 International GNOME Support (http://www.gnome-support.com)
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-object.h>
#include <bonobo/gnome-embeddable.h>

static GnomeObjectClass *gnome_embeddable_parent_class;

enum {
	HOST_NAME_CHANGED,
	URI_CHANGED,
	LAST_SIGNAL
};

static guint embeddable_signals [LAST_SIGNAL];

struct _GnomeEmbeddablePrivate {
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
	GnomeViewFactory view_factory;
	void *view_factory_closure;

	/*
	 * For the Canvas Item
	 */
	GnomeItemCreator item_creator;
	void *item_creator_data;
};

static void
impl_GNOME_Embeddable_set_client_site (PortableServer_Servant servant,
				       const GNOME_ClientSite client_site,
				       CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));
	CORBA_Environment evx;

	CORBA_exception_init (&evx);

	if (embeddable->client_site != CORBA_OBJECT_NIL)
		CORBA_Object_release (client_site, &evx);
	
	embeddable->client_site = CORBA_Object_duplicate (client_site, &evx);
        CORBA_exception_free (&evx);							     
}

static GNOME_ClientSite
impl_GNOME_Embeddable_get_client_site (PortableServer_Servant servant,
				       CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));
	GNOME_ClientSite ret;
	CORBA_Environment evx;
	
	CORBA_exception_init (&evx);
	ret = CORBA_Object_duplicate (embeddable->client_site, &evx);
        CORBA_exception_free (&evx);							     

	return ret;
}

static void
impl_GNOME_Embeddable_set_host_name (PortableServer_Servant servant,
				     CORBA_char *name,
				     CORBA_char *appname,
				     CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));

	if (embeddable->host_name)
		g_free (embeddable->host_name);
	if (embeddable->host_appname)
		g_free (embeddable->host_appname);

	embeddable->host_name = g_strdup (name);
	embeddable->host_appname = g_strdup (appname);

	gtk_signal_emit (GTK_OBJECT (embeddable),
			 embeddable_signals [HOST_NAME_CHANGED]);
}


static void
impl_GNOME_Embeddable_close (PortableServer_Servant servant,
			     const GNOME_Embeddable_CloseMode mode,
			     CORBA_Environment *ev)
{
}

static GNOME_Embeddable_verb_list *
impl_GNOME_Embeddable_get_verb_list (PortableServer_Servant servant,
				     CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));
	GNOME_Embeddable_verb_list *verb_list;

	GList *l;
	int len;
	int i;

	len = g_list_length (embeddable->verbs);

	verb_list = GNOME_Embeddable_verb_list__alloc ();
	verb_list->_length = len;

	if (len == 0)
		return verb_list;

	verb_list->_buffer = CORBA_sequence_GNOME_Embeddable_GnomeVerb_allocbuf (len);

	for (i = 0, l = embeddable->verbs; l != NULL; l = l->next, i ++) {
		GNOME_Embeddable_GnomeVerb *corba_verb;
		GnomeVerb *verb = (GnomeVerb *) l->data;

		corba_verb = & verb_list->_buffer [i];
#define CORBIFY_STRING(s) ((s) == NULL ? "" : (s))
		corba_verb->name = CORBA_string_dup (CORBIFY_STRING (verb->name));
		corba_verb->label = CORBA_string_dup (CORBIFY_STRING (verb->label));
		corba_verb->hint = CORBA_string_dup (CORBIFY_STRING (verb->hint));
	}

	return verb_list;
}

static void
impl_GNOME_Embeddable_advise (PortableServer_Servant servant,
			      const GNOME_AdviseSink advise,
			      CORBA_Environment *ev)
{
}

static void
impl_GNOME_Embeddable_unadvise (PortableServer_Servant servant, CORBA_Environment *ev)
{
}

static CORBA_long
impl_GNOME_Embeddable_get_misc_status (PortableServer_Servant servant,
				       const CORBA_long type,
				       CORBA_Environment *ev)
{

	return 0;
}

static void
ping_container (GnomeEmbeddable *embeddable)
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
		gnome_object_destroy (GNOME_OBJECT (embeddable));
	}
}

static void
gnome_embeddable_view_destroy_cb (GnomeView *view, gpointer data)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (data);

	/*
	 * Remove this view from our list of views.
	 */
	embeddable->priv->views = g_list_remove (embeddable->priv->views, view);

	ping_container (embeddable);
}

static GNOME_View
impl_GNOME_Embeddable_new_view (PortableServer_Servant servant,
				GNOME_ViewFrame view_frame,
				CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));
	GnomeView *view;
	CORBA_Environment evx;
	GNOME_View ret;
	
	view = embeddable->priv->view_factory (
		embeddable, view_frame,
		embeddable->priv->view_factory_closure);

	if (view == NULL)
		return CORBA_OBJECT_NIL;

	if (gnome_object_corba_objref (GNOME_OBJECT (view)) == CORBA_OBJECT_NIL){
		g_warning ("Returned view does not have a CORBA object bound\n");
		gtk_object_destroy (GTK_OBJECT (view));
		return CORBA_OBJECT_NIL;
	}
	gnome_view_set_view_frame (view, view_frame);
	gnome_view_set_embeddable (view, embeddable);

	embeddable->priv->views = g_list_prepend (embeddable->priv->views, view);

	gtk_signal_connect (GTK_OBJECT (view), "destroy",
			    GTK_SIGNAL_FUNC (gnome_embeddable_view_destroy_cb), embeddable);

	CORBA_exception_init (&evx);
	ret = CORBA_Object_duplicate (gnome_object_corba_objref (GNOME_OBJECT (view)), &evx);
	CORBA_exception_free (&evx);

	return ret;
}

static void
impl_GNOME_Embeddable_set_uri (PortableServer_Servant servant,
			       CORBA_char *uri,
			       CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));

	gnome_embeddable_set_uri (embeddable, uri);
}

static void
canvas_item_destroyed (GnomeCanvasComponent *comp, GnomeEmbeddable *embeddable)
{
	GnomeCanvasItem *item;

	item = gnome_canvas_component_get_item (comp);
	gtk_object_destroy (GTK_OBJECT (item->canvas));
	
	/*
	 * Remove the canvas item from the list of items we keep
	 */
	embeddable->priv->canvas_items = g_list_remove (embeddable->priv->canvas_items, comp);

	ping_container (embeddable);
}

static GnomeCanvasComponent *
make_canvas_component (GnomeEmbeddable *embeddable, gboolean aa, GNOME_Canvas_ItemProxy item_proxy)
{
	GnomeCanvasComponent *component;
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
	gnome_canvas_component_set_proxy (component, item_proxy);

	/*
	 * Now keep track of it
	 */
	embeddable->priv->canvas_items = g_list_prepend (embeddable->priv->canvas_items, component);
	gtk_signal_connect (GTK_OBJECT (component), "destroy",
			    GTK_SIGNAL_FUNC (canvas_item_destroyed), embeddable);
	
	return component;
}

static GNOME_Canvas_Item
impl_GNOME_Embeddable_new_canvas_item (PortableServer_Servant servant,
				       CORBA_boolean aa,
				       GNOME_Canvas_ItemProxy _item_proxy,
				       CORBA_Environment *ev)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (gnome_object_from_servant (servant));
	GNOME_Canvas_ItemProxy item_proxy;
	GnomeCanvasComponent *component;
	
	if (embeddable->priv->item_creator == NULL)
		return CORBA_OBJECT_NIL;

	item_proxy = CORBA_Object_duplicate (_item_proxy, ev);
	
	component = make_canvas_component (embeddable, aa, item_proxy);

	return CORBA_Object_duplicate (gnome_object_corba_objref (GNOME_OBJECT (component)), ev);
}

POA_GNOME_Embeddable__epv gnome_embeddable_epv = {
	NULL,
};

static POA_GNOME_Embeddable__vepv gnome_embeddable_vepv = {
	&gnome_object_base_epv,
	&gnome_object_epv,
	&gnome_embeddable_epv
};

static void
gnome_embeddable_corba_class_init ()
{
	/* the epv */
	gnome_embeddable_epv.set_client_site = &impl_GNOME_Embeddable_set_client_site;
	gnome_embeddable_epv.get_client_site = &impl_GNOME_Embeddable_get_client_site;
	gnome_embeddable_epv.set_host_name   = &impl_GNOME_Embeddable_set_host_name;
	gnome_embeddable_epv.close           = &impl_GNOME_Embeddable_close;
	gnome_embeddable_epv.get_verb_list   = &impl_GNOME_Embeddable_get_verb_list;
	gnome_embeddable_epv.advise          = &impl_GNOME_Embeddable_advise;
	gnome_embeddable_epv.unadvise        = &impl_GNOME_Embeddable_unadvise;
	gnome_embeddable_epv.get_misc_status = &impl_GNOME_Embeddable_get_misc_status;
	gnome_embeddable_epv.new_view        = &impl_GNOME_Embeddable_new_view;
	gnome_embeddable_epv.set_uri         = &impl_GNOME_Embeddable_set_uri;
	gnome_embeddable_epv.new_canvas_item = &impl_GNOME_Embeddable_new_canvas_item;
}

/**
 * gnome_embeddable_corba_object_create:
 * @object: The GtkObject that will wrap the CORBA object.
 *
 * Creates an activates the CORBA object that is wrapped
 * by the GnomeObject @object.
 *
 * Returns: An activated object reference to the created object or
 * %CORBA_OBJECT_NIL in case of failure.
 */
GNOME_Embeddable
gnome_embeddable_corba_object_create (GnomeObject *object)
{
	POA_GNOME_Embeddable *servant;
	CORBA_Environment ev;
	
	servant = (POA_GNOME_Embeddable *)g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_embeddable_vepv;

	CORBA_exception_init (&ev);

	POA_GNOME_Embeddable__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);

	return gnome_object_activate_servant (object, servant);
}

/**
 * gnome_embeddable_construct:
 * @embeddable: GnomeEmbeddable object to construct.
 * @corba_embeddable: The CORBA reference that implements this object.
 * @view_factory: Factory routine that provides new views of the embeddable on demand
 * @view_factory_data: pointer passed to the @view_factory routine to provide context.
 * @item_factory: A factory routine that creates GnomeCanvasComponents.
 * @item_factory_data: pointer passed to the @item_factory routine.
 *
 * This routine constructs a GNOME::Embeddable CORBA server and activates it.
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
GnomeEmbeddable *
gnome_embeddable_construct_full (GnomeEmbeddable *embeddable,
				 GNOME_Embeddable corba_embeddable,
				 GnomeViewFactory view_factory,
				 void             *factory_data,
				 GnomeItemCreator item_factory,
				 void             *item_factory_data)
{
	
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_EMBEDDABLE (embeddable), NULL);
	g_return_val_if_fail (corba_embeddable != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (embeddable), corba_embeddable);

	embeddable->priv->view_factory = view_factory;
	embeddable->priv->view_factory_closure = factory_data;
	embeddable->priv->item_creator = item_factory;
	embeddable->priv->item_creator_data = item_factory_data;
		
	return embeddable;
}

/**
 * gnome_embeddable_construct:
 * @embeddable: GnomeEmbeddable object to construct.
 * @corba_embeddable: The CORBA reference that implements this object.
 * @factory: Factory routine that provides new views of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 * 
 * This routine constructs a GNOME::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns: The constructed object.
 */
GnomeEmbeddable *
gnome_embeddable_construct (GnomeEmbeddable  *embeddable,
			    GNOME_Embeddable  corba_embeddable,
			    GnomeViewFactory factory,
			    void *data)
{
	return gnome_embeddable_construct_full (embeddable, corba_embeddable, factory, data, NULL, NULL);
}

/**
 * gnome_embeddable_new:
 * @factory: Factory routine that provides new views of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a GNOME::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a GnomeEmbeddable that contains an activated GNOME::Embeddable
 * CORBA server.
 */
GnomeEmbeddable *
gnome_embeddable_new (GnomeViewFactory factory, void *data)
{
	GNOME_Embeddable corba_embeddable;
	GnomeEmbeddable *embeddable;

	g_return_val_if_fail (factory != NULL, NULL);

	embeddable = gtk_type_new (GNOME_EMBEDDABLE_TYPE);

	corba_embeddable = gnome_embeddable_corba_object_create (GNOME_OBJECT (embeddable));
	if (corba_embeddable == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (embeddable));
		return NULL;
	}
	
	return gnome_embeddable_construct (embeddable, corba_embeddable, factory, data);
}

/**
 * gnome_embeddable_new_canvas_item:
 * @item_factory: Factory routine that provides new canvas items of the embeddable on demand
 * @data: pointer passed to the @factory routine to provide context.
 *
 * This routine creates a GNOME::Embeddable CORBA server and activates it.  The
 * @factory routine will be invoked by this CORBA server when a request arrives
 * to get a new view of the embeddable (embeddable should be able to provide
 * multiple views of themselves upon demand).  The @data pointer is passed
 * to this factory routine untouched to allow the factory to get some context
 * on what it should create.
 *
 * Returns a GnomeEmbeddable that contains an activated GNOME::Embeddable
 * CORBA server.
 */
GnomeEmbeddable *
gnome_embeddable_new_canvas_item (GnomeItemCreator item_factory, void *data)
{
	GNOME_Embeddable corba_embeddable;
	GnomeEmbeddable *embeddable;

	g_return_val_if_fail (item_factory != NULL, NULL);

	embeddable = gtk_type_new (GNOME_EMBEDDABLE_TYPE);

	corba_embeddable = gnome_embeddable_corba_object_create (GNOME_OBJECT (embeddable));
	if (corba_embeddable == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (embeddable));
		return NULL;
	}
	
	return gnome_embeddable_construct_full (embeddable, corba_embeddable, NULL, NULL, item_factory, data);
}

static void
gnome_embeddable_destroy (GtkObject *object)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (object);
	GList *l;

	/*
	 * Destroy all our views.
	 */
	while (embeddable->priv->views) {
		GnomeView *view = GNOME_VIEW (embeddable->priv->views->data);

		gnome_object_destroy (GNOME_OBJECT (view));
	}

	while (embeddable->priv->canvas_items){
		void *data = embeddable->priv->canvas_items->data;
		GnomeCanvasComponent *comp = GNOME_CANVAS_COMPONENT (data);

		gnome_object_destroy (GNOME_OBJECT (comp));
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
	
	GTK_OBJECT_CLASS (gnome_embeddable_parent_class)->destroy (object);
}

static void
gnome_embeddable_class_init (GnomeEmbeddableClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_embeddable_parent_class =
		gtk_type_class (gnome_object_get_type ());

	embeddable_signals [HOST_NAME_CHANGED] =
                gtk_signal_new ("host_name_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeEmbeddableClass,host_name_changed), 
                                gtk_marshal_NONE__STRING,
                                GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
	embeddable_signals [URI_CHANGED] =
                gtk_signal_new ("uri_changed",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET(GnomeEmbeddableClass,uri_changed), 
                                gtk_marshal_NONE__STRING,
                                GTK_TYPE_NONE, 1, GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, embeddable_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_embeddable_destroy;

	gnome_embeddable_corba_class_init ();
}

static void
gnome_embeddable_init (GnomeObject *object)
{
	GnomeEmbeddable *embeddable = GNOME_EMBEDDABLE (object);

	embeddable->priv = g_new0 (GnomeEmbeddablePrivate, 1);
}

/**
 * gnome_embeddable_get_type:
 *
 * Returns: The GtkType for the GnomeEmbeddable class.
 */
GtkType
gnome_embeddable_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/Embeddable:1.0",
			sizeof (GnomeEmbeddable),
			sizeof (GnomeEmbeddableClass),
			(GtkClassInitFunc) gnome_embeddable_class_init,
			(GtkObjectInitFunc) gnome_embeddable_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_embeddable_set_view_factory:
 * @embeddable: The embeddable object to operate on.
 * @factory: A pointer to a function that can provide GnomeView objects on demand.
 * @data: data to pass to the @factory function.
 *
 * This routine defines the view factory for this embeddable component.
 * When a container requires a view, the routine specified in @factory
 * will be invoked to create a new GnomeView object to satisfy this request.
 */
void
gnome_embeddable_set_view_factory (GnomeEmbeddable *embeddable,
				   GnomeViewFactory factory,
				   void *data)
{
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (factory != NULL);

	embeddable->priv->view_factory = factory;
	embeddable->priv->view_factory_closure = data;
}

/**
 * gnome_embeddable_add_verb:
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
gnome_embeddable_add_verb (GnomeEmbeddable *embeddable,
			   const char *verb_name, const char *verb_label, const char *verb_hint)
{
	GnomeVerb *verb;

	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (verb_name != NULL);

	verb = g_new0 (GnomeVerb, 1);
	verb->name = g_strdup (verb_name);
	verb->label = g_strdup (verb_label);
	verb->hint = g_strdup (verb_hint);

	embeddable->verbs = g_list_prepend (embeddable->verbs, verb);
}

/**
 * gnome_embeddable_add_verbs:
 * @embeddable: The embeddable object to operate on.
 * @verbs: An array of GnomeVerbs to be added.
 *
 * This routine adds the list of verbs in the NULL terminated array
 * in @verbs to the exported verbs for the component.
 */
void
gnome_embeddable_add_verbs (GnomeEmbeddable *embeddable, const GnomeVerb *verbs)
{
	int i;

	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (verbs != NULL);

	for (i = 0; verbs [i].name != NULL; i++)
		gnome_embeddable_add_verb (embeddable, verbs[i].name, verbs[i].label, verbs[i].hint);
}

/**
 * gnome_embeddable_remove_verb:
 * @embeddable: The embeddable object to operate on.
 * @verb_name: a verb name
 *
 * This routine removes the verb called @verb_name from the list
 * of exported verbs for this embeddable object
 */
void
gnome_embeddable_remove_verb (GnomeEmbeddable *embeddable, const char *verb_name)
{
	GList *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
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
 * gnome_embeddable_get_verbs:
 * @embeddable: A GnomeEmbeddable object.
 *
 * Returns the internal copy of the list of verbs supported by this
 * Embeddable object.
 */
const GList *
gnome_embeddable_get_verbs (GnomeEmbeddable *embeddable)
{
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_EMBEDDABLE (embeddable), NULL);

	return (const GList *) embeddable->verbs;
}


/**
 * gnome_embeddable_get_uri:
 * @embeddable: The embeddable object to operate on.
 *
 * Returns the URI that this object represents
 */
const char *
gnome_embeddable_get_uri (GnomeEmbeddable *embeddable)
{
	g_return_val_if_fail (embeddable != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_EMBEDDABLE (embeddable), NULL);

	return embeddable->uri;
}

/**
 * gnome_embeddable_set_uri:
 * @embeddable: The embeddable object to operate on.
 * @uri: the URI this embeddable represents.
 *
 * Sets the URI that this object represents.
 */
void
gnome_embeddable_set_uri (GnomeEmbeddable *embeddable, const char *uri)
{
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));

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
 * gnome_embeddable_foreach_view:
 * @embeddable: Embeddable on which we operate
 * @fn: function to be invoked for each existing GnomeView
 * @data: data to pass to function
 *
 * Invokes the @fn function for each view existing
 */
void
gnome_embeddable_foreach_view (GnomeEmbeddable *embeddable,
			       GnomeEmbeddableForeachViewFn fn,
			       void *data)
{
	GList *copy, *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (fn != NULL);

	copy = g_list_copy (embeddable->priv->views);
	for (l = copy; l; l = l->next)
		(*fn)(GNOME_VIEW (l->data), data);

	g_list_free (copy);
}

/**
 * gnome_embeddable_foreach_item:
 * @embeddable: Embeddable on which we operate
 * @fn: function to be invoked for each existing GnomeItem
 * @data: data to pass to function
 *
 * Invokes the @fn function for each item existing
 */
void
gnome_embeddable_foreach_item (GnomeEmbeddable *embeddable,
			       GnomeEmbeddableForeachItemFn fn,
			       void *data)
{
	GList *copy, *l;
	
	g_return_if_fail (embeddable != NULL);
	g_return_if_fail (GNOME_IS_EMBEDDABLE (embeddable));
	g_return_if_fail (fn != NULL);

	copy = g_list_copy (embeddable->priv->canvas_items);
	for (l = copy; l; l = l->next)
		(*fn)(GNOME_CANVAS_COMPONENT (l->data), data);

	g_list_free (copy);
}
