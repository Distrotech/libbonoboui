/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-component-ui.c: Client UI signal multiplexer and verb repository.
 *
 * Author:
 *   Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <gnome.h>
#include <bonobo.h>
#include <bonobo/bonobo-ui-component.h>

static BonoboObjectClass *bonobo_ui_component_parent_class;
enum {
	EXEC_VERB,
	UI_EVENT,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

POA_Bonobo_UIComponent__vepv bonobo_ui_component_vepv;

typedef struct {
	char              *id;
	BonoboUIListenerFn cb;
	gpointer           user_data;
} Listener;

/*
 * FIXME: should all be hashed for speed.
 */
struct _BonoboUIComponentPrivate {
	GSList *verbs;
	GSList *listeners;
	char   *name;
};

static inline BonoboUIComponent *
bonobo_ui_from_servant (PortableServer_Servant servant)
{
	return BONOBO_UI_COMPONENT (bonobo_object_from_servant (servant));
}

static void
verb_destroy (BonoboUIVerb *verb)
{
	if (verb) {
		g_free (verb->cname);
		g_free (verb);
	}
}

static void
listener_destroy (Listener *l)
{
	if (l) {
		g_free (l->id);
		g_free (l);
	}
}

static void
ui_event (BonoboUIComponent           *component,
	  const char                  *id,
	  Bonobo_UIComponent_EventType type,
	  const char                  *state)
{
	GSList *l;

	for (l = component->priv->listeners; l; l = l->next) {
		Listener *list = l->data;

		if (!strcmp (list->id, id)) {
			list->cb (component, id, type,
				  state, list->user_data);
		}
	}
}

static CORBA_char *
impl_describe_verbs (PortableServer_Servant servant,
		     CORBA_Environment     *ev)
{
	g_warning ("FIXME: Describe verbs unimplemented");
	return CORBA_string_dup ("<NoBonoboUIVerbDescriptionCodeYet/>");
}

static void
impl_exec_verb (PortableServer_Servant servant,
		const CORBA_char      *cname,
		CORBA_Environment     *ev)
{
	GSList *l;
	gboolean found = FALSE;
	BonoboUIComponent *component;

	component = bonobo_ui_from_servant (servant);

	bonobo_object_ref (BONOBO_OBJECT (component));
	
/*	g_warning ("TESTME: Exec verb '%s'", cname);*/

	for (l = component->priv->verbs; l; l = l->next) {
		BonoboUIVerb *verb = l->data;

		if (!strcmp (verb->cname, cname)) {
			if (verb->cb)
				verb->cb (component, verb->user_data, cname);
			found = TRUE;
		}
	}
	if (!found) {
		g_warning ("FIXME: verb '%s' not found, emit exception",
			   cname);
	}

	gtk_signal_emit (GTK_OBJECT (component),
			 signals [EXEC_VERB],
			 cname);

	bonobo_object_unref (BONOBO_OBJECT (component));
}

static void
impl_ui_event (PortableServer_Servant             servant,
	       const CORBA_char                  *id,
	       const Bonobo_UIComponent_EventType type,
	       const CORBA_char                  *state,
	       CORBA_Environment                 *ev)
{
	BonoboUIComponent *component;

	component = bonobo_ui_from_servant (servant);

/*	g_warning ("TESTME: Event '%s' '%d' '%s'\n", path, type, state);*/

	bonobo_object_ref (BONOBO_OBJECT (component));

	gtk_signal_emit (GTK_OBJECT (component),
			 signals [UI_EVENT], id, type, state);

	bonobo_object_unref (BONOBO_OBJECT (component));
}


void
bonobo_ui_component_add_verb (BonoboUIComponent  *component,
			      const char         *cname,
			      BonoboUIVerbFn      fn,
			      gpointer            user_data)
{
	BonoboUIVerb *verb;
	BonoboUIComponentPrivate *priv;

	g_return_if_fail (cname != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (component));

	verb = g_new (BonoboUIVerb, 1);
	verb->cname = g_strdup (cname);
	verb->cb        = fn;
	verb->user_data = user_data;

	priv = component->priv;

	priv->verbs = g_slist_prepend (priv->verbs, verb);
}

void
bonobo_ui_component_add_listener (BonoboUIComponent  *component,
				  const char         *id,
				  BonoboUIListenerFn  fn,
				  gpointer            user_data)
{
	Listener *list;
	BonoboUIComponentPrivate *priv;

	g_return_if_fail (fn != NULL);
	g_return_if_fail (id != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (component));

	list = g_new (Listener, 1);
	list->cb = fn;
	list->id = g_strdup (id);
	list->user_data = user_data;

	priv = component->priv;
	priv->listeners = g_slist_prepend (priv->listeners, list);	
}

static void
bonobo_ui_component_destroy (GtkObject *object)
{
	BonoboUIComponent *comp = (BonoboUIComponent *) object;
	BonoboUIComponentPrivate *priv = comp->priv;

	if (priv) {
		GSList *l;

		for (l = priv->verbs; l; l = l->next)
			verb_destroy (l->data);
		g_slist_free (priv->verbs);

		for (l = priv->listeners; l; l = l->next)
			listener_destroy (l->data);
		g_slist_free (priv->listeners);

		g_free (priv->name);

		g_free (priv);
	}
	comp->priv = NULL;
}

POA_Bonobo_UIComponent__epv *
bonobo_ui_component_get_epv (void)
{
	POA_Bonobo_UIComponent__epv *epv;

	epv = g_new0 (POA_Bonobo_UIComponent__epv, 1);

	epv->describe_verbs = impl_describe_verbs;
	epv->exec_verb = impl_exec_verb;
	epv->ui_event  = impl_ui_event;

	return epv;
}

static void
bonobo_ui_component_class_init (BonoboUIComponentClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;
	BonoboUIComponentClass *uclass = BONOBO_UI_COMPONENT_CLASS (klass);
	
	bonobo_ui_component_parent_class = gtk_type_class (BONOBO_OBJECT_TYPE);

	object_class->destroy = bonobo_ui_component_destroy;

	uclass->ui_event = ui_event;

	bonobo_ui_component_vepv.Bonobo_Unknown_epv =
		bonobo_object_get_epv ();
	bonobo_ui_component_vepv.Bonobo_UIComponent_epv =
		bonobo_ui_component_get_epv ();

	signals [EXEC_VERB] = gtk_signal_new (
		"exec_verb", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIComponentClass, exec_verb),
		gtk_marshal_NONE__STRING,
		GTK_TYPE_NONE, 1, GTK_TYPE_STRING);

	signals [UI_EVENT] = gtk_signal_new (
		"ui_event", GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (BonoboUIComponentClass, ui_event),
		gtk_marshal_NONE__POINTER_INT_POINTER,
		GTK_TYPE_NONE, 3, GTK_TYPE_STRING, GTK_TYPE_INT,
		GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
bonobo_ui_component_init (BonoboUIComponent *component)
{
	component->priv = g_new0 (BonoboUIComponentPrivate, 1);
}

/**
 * bonobo_ui_component_get_type:
 *
 * Returns: the GtkType of the BonoboUIComponent class.
 */
GtkType
bonobo_ui_component_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboUIComponent",
			sizeof (BonoboUIComponent),
			sizeof (BonoboUIComponentClass),
			(GtkClassInitFunc) bonobo_ui_component_class_init,
			(GtkObjectInitFunc) bonobo_ui_component_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

Bonobo_UIComponent
bonobo_ui_component_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_UIComponent *servant;
	CORBA_Environment       ev;

	servant = (POA_Bonobo_UIComponent *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_ui_component_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_UIComponent__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
		CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

	CORBA_exception_free (&ev);

	return bonobo_object_activate_servant (object, servant);
}

BonoboUIComponent *
bonobo_ui_component_construct (BonoboUIComponent *ui_component,
			       Bonobo_UIComponent corba_component,
			       const char        *name)
{
	g_return_val_if_fail (corba_component != CORBA_OBJECT_NIL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_COMPONENT (ui_component), NULL);

	ui_component->priv->name = g_strdup (name);

	return BONOBO_UI_COMPONENT (
		bonobo_object_construct (BONOBO_OBJECT (ui_component),
					 corba_component));
}

BonoboUIComponent *
bonobo_ui_component_new (const char *name)
{
	BonoboUIComponent *component;
	Bonobo_UIComponent corba_component;

	component = gtk_type_new (BONOBO_UI_COMPONENT_TYPE);
	if (component == NULL)
		return NULL;

	corba_component = bonobo_ui_component_corba_object_create (
		BONOBO_OBJECT (component));

	if (corba_component == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (component));
		return NULL;
	}

	return BONOBO_UI_COMPONENT (bonobo_ui_component_construct (
		component, corba_component, name));
}

void
bonobo_ui_component_set (BonoboUIComponent  *component,
			 Bonobo_UIContainer  container,
			 const char         *path,
			 const char         *xml,
			 CORBA_Environment  *ev)
{
	BonoboUIComponentPrivate *priv;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_if_fail (container != CORBA_OBJECT_NIL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (component));

/*	fprintf (stderr, "Set '%s'\n", xml);*/

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	priv = component->priv;

	Bonobo_UIContainer_register_component (
		container, priv->name,
		bonobo_object_corba_objref (BONOBO_OBJECT (component)),
		real_ev);

	if (real_ev->_major != CORBA_NO_EXCEPTION && !ev)
		g_warning ("Serious exception registering component '$%s'",
			   bonobo_exception_get_text (real_ev));

	Bonobo_UIContainer_node_set (container, path, xml,
				     priv->name, real_ev);

	if (real_ev->_major != CORBA_NO_EXCEPTION && !ev)
		g_warning ("Serious exception on node_set '$%s'",
			   bonobo_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);
}

void
bonobo_ui_component_set_tree (BonoboUIComponent  *component,
			      Bonobo_UIContainer  container,
			      const char         *path,
			      xmlNode            *node,
			      CORBA_Environment  *ev)
{
	xmlDoc     *doc;
	xmlChar    *mem = NULL;
	int         size;

	doc = xmlNewDoc ("1.0");
	g_return_if_fail (doc != NULL);

	doc->root = node;

	xmlDocDumpMemory (doc, &mem, &size);

	g_return_if_fail (mem != NULL);

	doc->root = NULL;
	xmlFreeDoc (doc);

/*	fprintf (stderr, "Merging '%s'\n", mem);*/
	
	bonobo_ui_component_set (
		component, container, path, mem, ev);

	xmlFree (mem);
}

char *
bonobo_ui_container_get (Bonobo_UIContainer  container,
			 const char         *path,
			 gboolean            recurse,
			 CORBA_Environment  *ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	CORBA_char *xml;

	g_return_val_if_fail (container != CORBA_OBJECT_NIL, NULL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	xml = Bonobo_UIContainer_node_get (container, path, recurse, real_ev);

	if (real_ev->_major != CORBA_NO_EXCEPTION) {
		if (!ev)
			g_warning ("Serious exception getting node '%s' '$%s'",
				   path, bonobo_exception_get_text (real_ev));
		return NULL;
	}

	return xml;
}

xmlNode *
bonobo_ui_container_get_tree (Bonobo_UIContainer  container,
			      const char         *path,
			      gboolean            recurse,
			      CORBA_Environment  *ev)
{	
	char *xml = bonobo_ui_container_get (container, path, recurse, ev);
	xmlNode *node;
	xmlDoc  *doc;

	if (!xml)
		return NULL;

	doc = xmlParseDoc ((char *)xml);

	if (!doc)
		return NULL;

	node = doc->root;
	doc->root = NULL;
	
	xmlFreeDoc (doc);

	bonobo_ui_xml_strip (node);

	return node;
}

void
bonobo_ui_component_rm (BonoboUIComponent  *component,
			Bonobo_UIContainer  container,
			const char         *path,
			CORBA_Environment  *ev)
{
	BonoboUIComponentPrivate *priv;
	CORBA_Environment *real_ev, tmp_ev;

	g_return_if_fail (container != CORBA_OBJECT_NIL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (component));

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	priv = component->priv;

	Bonobo_UIContainer_deregister_component (
		container, priv->name, real_ev);

	if (!ev && real_ev->_major != CORBA_NO_EXCEPTION)
		g_warning ("Serious exception removing path  '%s' '%s'",
			   path, bonobo_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);
}


void
bonobo_ui_container_object_set (Bonobo_UIContainer  container,
				const char         *path,
				Bonobo_Unknown      control,
				CORBA_Environment  *ev)
{
	CORBA_Environment *real_ev, tmp_ev;

	g_return_if_fail (container != CORBA_OBJECT_NIL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	Bonobo_UIContainer_object_set (container, path, control, real_ev);

	if (!ev && real_ev->_major != CORBA_NO_EXCEPTION)
		g_warning ("Serious exception setting object '%s' '%s'",
			   path, bonobo_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);
}

Bonobo_Unknown
bonobo_ui_container_object_get (Bonobo_UIContainer  container,
				const char         *path,
				CORBA_Environment  *ev)
{
	CORBA_Environment *real_ev, tmp_ev;
	Bonobo_Unknown     ret;

	g_return_val_if_fail (container != CORBA_OBJECT_NIL,
			      CORBA_OBJECT_NIL);

	if (ev)
		real_ev = ev;
	else {
		CORBA_exception_init (&tmp_ev);
		real_ev = &tmp_ev;
	}

	ret = Bonobo_UIContainer_object_get (container, path, real_ev);

	if (!ev && real_ev->_major != CORBA_NO_EXCEPTION)
		g_warning ("Serious exception getting object '%s' '%s'",
			   path, bonobo_exception_get_text (real_ev));

	if (!ev)
		CORBA_exception_free (&tmp_ev);

	return ret;
}

void
bonobo_ui_component_add_verb_list_with_data (BonoboUIComponent  *component,
					     BonoboUIVerb       *list,
					     gpointer            user_data)
{
	BonoboUIVerb *l;

	g_return_if_fail (list != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (component));

	for (l = list; l && l->cname; l++) {
		bonobo_ui_component_add_verb (
			component, l->cname, l->cb,
			user_data?user_data:l->user_data);
	}
}

void
bonobo_ui_component_add_verb_list (BonoboUIComponent  *component,
				   BonoboUIVerb       *list)
{
	bonobo_ui_component_add_verb_list_with_data (component, list, NULL);
}
