/*
 *  This module acts as an excercise in filthy coding habits
 * take a look around.
 */

#include "bonobo.h"
#include "bonobo-ui-compat.h"

struct _BonoboUIHandlerPrivate {
	BonoboUIComponent *component;
	BonoboApp         *app;

	char              *name;

	BonoboUIXml       *ui;

	Bonobo_UIContainer container;
};

static int busk_name = 0;

#define MAGIC_UI_HANDLER_KEY "Bonobo::CompatUIKey"

static BonoboUIHandlerPrivate *
get_priv (BonoboUIHandler *uih)
{
	return gtk_object_get_data (GTK_OBJECT (uih), MAGIC_UI_HANDLER_KEY);
}

void
bonobo_ui_handler_set_container (BonoboUIHandler *uih,
				 Bonobo_Unknown   cont)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);
	/* blah */

	priv->container = bonobo_object_dup_ref (cont, NULL);
	
	bonobo_ui_component_set_tree (priv->component, priv->container,
				      "/", priv->ui->root, NULL);
}

void
bonobo_ui_handler_unset_container (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

	if (priv->container != CORBA_OBJECT_NIL) {
		bonobo_ui_component_rm (priv->component, priv->container, "/", NULL);
		bonobo_object_release_unref (priv->container, NULL);
	}
	priv->container = CORBA_OBJECT_NIL;
}

void
bonobo_ui_handler_create_menubar (BonoboUIHandler *uih)
{
	BonoboUIHandlerPrivate *priv = get_priv (uih);

#warning FIXME; we currently always have a menu bar; hmmm...
	bonobo_ui_xml_get_path (priv->ui, "/menu");
}

static void
priv_destroy (GtkObject *object, BonoboUIHandlerPrivate *priv)
{
	if (priv->container)
		bonobo_ui_handler_unset_container ((BonoboUIHandler *)object);

	gtk_object_unref (GTK_OBJECT (priv->ui));

	g_free (priv->name);
	g_free (priv);
}

static void
setup_priv (GtkObject *object)
{
	BonoboUIHandlerPrivate *priv;

	priv = g_new0 (BonoboUIHandlerPrivate, 1);

	priv->ui = bonobo_ui_xml_new (NULL, NULL, NULL, NULL, NULL);

	if (BONOBO_IS_APP (object))
		priv->app = BONOBO_APP (object);
	else
		priv->component = BONOBO_UI_COMPONENT (object);

	gtk_object_set_data (GTK_OBJECT (object), MAGIC_UI_HANDLER_KEY, priv);
	gtk_signal_connect  (GTK_OBJECT (object), "destroy",
			     (GtkSignalFunc) priv_destroy, priv);
}

/*
 * This constructs a new component client
 */
BonoboUIHandler *
bonobo_ui_handler_new (void)
{
	GtkObject *object;
	char      *name = g_strdup_printf ("Busk%d", busk_name++);

	object = GTK_OBJECT (bonobo_ui_component_new (name));
	g_free (name);

	setup_priv (object);

	return (BonoboUIHandler *)object;
}

/*
 * This constructs a new container
 */
BonoboUIHandler *
bonobo_ui_handler_new_app (void)
{
	BonoboApp         *app;
	BonoboUIHandlerPrivate *priv;

	app = bonobo_app_new ("foo", "baa");

	setup_priv (GTK_OBJECT (app));
	priv = get_priv ((BonoboUIHandler *)app);
	priv->component = bonobo_ui_component_new ("toplevel");

	/* Festering lice */
	bonobo_ui_handler_set_container (
		(BonoboUIHandler *) app, bonobo_object_corba_objref (BONOBO_OBJECT (app)));

	return (BonoboUIHandler *)app;
}
