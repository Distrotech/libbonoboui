#include "config.h"
#include <gnome.h>
#include <bonobo.h>
#include <liboaf/liboaf.h>

#include "Bonobo.h"
#include "bonobo-ui-xml.h"
#include "bonobo-ui-util.h"
#include "bonobo-app.h"
#include "bonobo-app-item.h"
#include "bonobo-app-toolbar.h"

GtkObjectClass *bonobo_app_parent_class = NULL;

POA_Bonobo_App__vepv bonobo_app_vepv;

/*
 * Implement placeholders & default placeholders
 * Test menu ordering; ensure correctness.
 * Accelerators.
 */

/*
 * dock layout; docka 0,3,5
 *              dockb 7,8,5 etc.
 *
 */


/*
 * Parent: Dock -> Container -> toolbar items.
 */
#define DO_GUI

struct _BonoboAppPrivate {
	GtkWidget     *window;
	GnomeDock     *dock;

	GnomeDockItem *menu_item;
	GtkMenuBar    *menu;

	GnomeDockItem *status_item;
	GtkBox        *status;

	BonoboUIXml   *tree;

	GtkAccelGroup *accel_group;

	char          *name;		/* App name */
	char          *prefix;		/* App prefix */
};

typedef struct {
	BonoboUIXmlData parent;

	GtkWidget      *widget;
} NodeInfo;

static gboolean
info_compare_fn (BonoboUIXmlData *a,
		 BonoboUIXmlData *b)
{
	/* FIXME: CORBA_Object_equivalent */
	return (a == b);
}

static BonoboUIXmlData *
info_new_fn (void)
{
	return (BonoboUIXmlData *)g_new0 (NodeInfo, 1);
}

static void
info_free_fn (BonoboUIXmlData *data)
{
	g_free (data);
}

static GtkWidget *
node_get_widget (BonoboUIXml *tree, xmlNode *node)
{
	NodeInfo *info;

	if (!node)
		return NULL;

	info = bonobo_ui_xml_get_data (tree, node);

	return info->widget;
}

static void
override_fn (GtkObject *object, xmlNode *node, gpointer dummy)
{
	char *str = bonobo_ui_xml_get_path (node);

	fprintf (stderr, "Override '%s'\n", str);
	g_free (str);
}

static void
reinstate_fn (GtkObject *object, xmlNode *node, gpointer dummy)
{
	char *str = bonobo_ui_xml_get_path (node);

	fprintf (stderr, "Reinstate '%s'\n", str);
	g_free (str);
}

static void
container_destroy_children (GtkContainer *container)
{
	GList *l;
	GList *children =
		gtk_container_children (
			GTK_CONTAINER (container));

	for (l = children; l; l = l->next)
		gtk_widget_destroy (l->data);

	g_list_free (children);
}

/*
 * see menu_toplevel_item_create_widget.
 */
static void
build_menu_widget (BonoboAppPrivate *priv, xmlNode *node)
{
	NodeInfo  *info;
	GtkWidget *parent, *menu_widget;
	char      *label_text;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_widget (priv->tree, node->parent);

	/* Create menu item */
	if (xmlGetProp (node, "pixtype")) {
		GtkWidget *pixmap;

		menu_widget = gtk_pixmap_menu_item_new ();

		pixmap = bonobo_ui_util_xml_get_pixmap (menu_widget, node);

		gtk_widget_show (GTK_WIDGET (pixmap));
		gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_widget),
						 GTK_WIDGET (pixmap));
	} else
		menu_widget = gtk_menu_item_new ();
	/*
	 * FIXME: (toplevel_create_item_widget)
	 *   Placeholder; Separator;
	 *   Radioitem, toggleitem
	 */ 

	if ((label_text = xmlGetProp (node, "label"))) {
		GtkWidget *label;
		guint      keyval;

		label = gtk_accel_label_new (label_text);

		/*
		 * Setup the widget.
		 */
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_widget_show (label);
		
		/*
		 * Insert it into the menu item widget and setup the
		 * accelerator.
		 */
		gtk_container_add (GTK_CONTAINER (menu_widget), label);
		gtk_accel_label_set_accel_widget (
			GTK_ACCEL_LABEL (label), menu_widget);
	
		keyval = gtk_label_parse_uline (GTK_LABEL (label), label_text);

		if (keyval != GDK_VoidSymbol) {
			if (GTK_IS_MENU (parent))
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					gtk_menu_ensure_uline_accel_group (
						GTK_MENU (parent)),
					keyval, 0, 0);

			else if (GTK_IS_MENU_BAR (parent) &&
			    priv->accel_group != NULL)
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					priv->accel_group,
					keyval, GDK_MOD1_MASK, 0);
			else
				g_warning ("Adding accelerator went bananas");
		}
	}

	if (!strcmp (node->name, "submenu")) {
		xmlNode      *l;
		GtkMenuShell *shell;
		GtkMenu      *menu;
		GtkWidget    *tearoff;

		if (parent == NULL)
			shell = GTK_MENU_SHELL (priv->menu);
		else
			shell = GTK_MENU_SHELL (parent);

		/* Create the menu shell. */
		menu = GTK_MENU (gtk_menu_new ());

		gtk_menu_set_accel_group (menu, priv->accel_group);

		/*
		 * Create the tearoff item at the beginning of the menu shell,
		 * if appropriate.
		 */
		if (gnome_preferences_get_menus_have_tearoff ()) {
			tearoff = gtk_tearoff_menu_item_new ();
			gtk_widget_show (tearoff);
			gtk_menu_prepend (GTK_MENU (menu), tearoff);
		}

		/*
		 * Associate this menu shell with the menu item for
		 * this submenu.
		 */
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget),
					   GTK_WIDGET (menu));

		info->widget = GTK_WIDGET (menu);

		/*
		 * FIXME:
		 *   Connect signal for cmd
		 *   Impl sensitivity
		 *   install_global_accelerators: This is wierd ...
		 *   Set toggle state for toggles & radios.
		 */
		gtk_menu_shell_append (shell, menu_widget);
		
		for (l = node->childs; l; l = l->next)
			build_menu_widget (priv, l);

	} else if (!strcmp (node->name, "menuitem")) {
		g_return_if_fail (parent != NULL);

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else {
		g_warning ("FIXME: Lower widget creation");
		return;
	}

	
/* FIXME: Emit verb here.
  gtk_signal_connect (GTK_OBJECT (menu_widget), "activated",
	emit_verb, );*/
}

static void
update_menus (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode  *l;
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	if (info->widget)
		container_destroy_children (GTK_CONTAINER (info->widget));
	else
		info->widget = GTK_WIDGET (priv->menu);

	for (l = node->childs; l; l = l->next)
		build_menu_widget (priv, l);
}

/*
 * see menu_toplevel_item_create_widget.
 */
static void
build_toolbar_widget (BonoboAppPrivate *priv, xmlNode *node)
{
	NodeInfo  *info;
	GtkWidget *parent, *menu_widget;

	g_return_if_fail (priv != NULL);
	g_return_if_fail (node != NULL);

	info = bonobo_ui_xml_get_data (priv->tree, node);

	parent = node_get_widget (priv->tree, node->parent);

	/* Create toolbar item */
	if (xmlGetProp (node, "pixtype")) {
		GtkWidget *pixmap;
		GtkWidget *item;

		menu_widget = gtk_pixmap_menu_item_new ();

		pixmap = bonobo_ui_util_xml_get_pixmap (parent, node);

		gtk_widget_show (GTK_WIDGET (pixmap));
/*		bonobo_app_toolbar_add (BONOBO_APP_TOOLBAR (parent),
		pixmap, "Kippers");*/

		item = bonobo_app_item_new_item (xmlGetProp (node, "label"),
						 pixmap);
		bonobo_app_toolbar_add (BONOBO_APP_TOOLBAR (parent),
					item, xmlGetProp (node, "descr"));
	}

	/*
	 * FIXME: (toplevel_create_item_widget)
	 *   Placeholder; Separator;
	 *   Radioitem, toggleitem
	 */ 

/*
	case BONOBO_UI_HANDLER_TOOLBAR_CONTROL:
		toolbar_item = bonobo_widget_new_control_from_objref (
			internal->item->control, CORBA_OBJECT_NIL);

		gtk_widget_show (toolbar_item);

		if (internal->item->pos >= 0)
			gtk_toolbar_insert_widget (GTK_TOOLBAR (toolbar),
						   toolbar_item,
						   internal->item->hint, NULL,
						   internal->item->pos);
		else
			gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar),
						   toolbar_item,
						   internal->item->hint, NULL);

		break;

	case BONOBO_UI_HANDLER_TOOLBAR_SEPARATOR:
		gtk_toolbar_insert_space (GTK_TOOLBAR (toolbar), internal->item->pos);
		break;

	case BONOBO_UI_HANDLER_TOOLBAR_ITEM:
	case BONOBO_UI_HANDLER_TOOLBAR_TOGGLEITEM:

		pixmap = NULL;
		if (internal->item->pixmap_data != NULL && internal->item->pixmap_type != BONOBO_UI_HANDLER_PIXMAP_NONE)
			pixmap = bonobo_ui_handler_toplevel_create_pixmap (
				toolbar, internal->item->pixmap_type, internal->item->pixmap_data);


		if (internal->item->pos >= 0)
			toolbar_item = gtk_toolbar_insert_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, pixmap,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal, internal->item->pos);
		else
			toolbar_item = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
								   toolbar_type_to_gtk_type (internal->item->type),
								   NULL, internal->item->label, internal->item->hint,
								   NULL, pixmap,
								   GTK_SIGNAL_FUNC (toolbar_toplevel_item_activated),
								   internal);

		break;

	case BONOBO_UI_HANDLER_TOOLBAR_RADIOITEM:
		g_warning ("Toolbar radio items/groups not implemented");
		return;
*/		


/*	if ((label_text = xmlGetProp (node, "label"))) {
		GtkWidget *label;
		guint      keyval;

		label = gtk_label_new (label_text);

		gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
		gtk_widget_show (label);
		
		gtk_container_add (GTK_CONTAINER (menu_widget), label);
		gtk_accel_label_set_accel_widget (
			GTK_ACCEL_LABEL (label), menu_widget);
	
		keyval = gtk_label_parse_uline (GTK_LABEL (label), label_text);

		if (keyval != GDK_VoidSymbol) {
			if (GTK_IS_MENU (parent))
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					gtk_menu_ensure_uline_accel_group (
						GTK_MENU (parent)),
					keyval, 0, 0);

			else if (GTK_IS_MENU_BAR (parent) &&
			    priv->accel_group != NULL)
				gtk_widget_add_accelerator (
					menu_widget, "activate_item",
					priv->accel_group,
					keyval, GDK_MOD1_MASK, 0);
			else
				g_warning ("Adding accelerator went bananas");
		}
		}*/

/*	if (!strcmp (node->name, "submenu")) {
		xmlNode      *l;
		GtkMenuShell *shell;
		GtkMenu      *menu;
		GtkWidget    *tearoff;

		if (parent == NULL)
			shell = GTK_MENU_SHELL (priv->menu);
		else
			shell = GTK_MENU_SHELL (parent);

		menu = GTK_MENU (gtk_menu_new ());

		gtk_menu_set_accel_group (menu, priv->accel_group);

		if (gnome_preferences_get_menus_have_tearoff ()) {
			tearoff = gtk_tearoff_menu_item_new ();
			gtk_widget_show (tearoff);
			gtk_menu_prepend (GTK_MENU (menu), tearoff);
		}

		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_widget),
					   GTK_WIDGET (menu));

		info->widget = GTK_WIDGET (menu);
*/
		/*
		 * FIXME:
		 *   Connect signal for cmd
		 *   Impl sensitivity
		 *   install_global_accelerators: This is wierd ...
		 *   Set toggle state for toggles & radios.
		 */
/*		gtk_menu_shell_append (shell, menu_widget);
		
		for (l = node->childs; l; l = l->next)
			build_menu_widget (priv, l);

	} else if (!strcmp (node->name, "menuitem")) {
		g_return_if_fail (parent != NULL);

		gtk_menu_shell_append (GTK_MENU_SHELL (parent), menu_widget);
	} else
	g_warning ("FIXME: Lower widget creation");*/
}

static void
update_dockitem (BonoboAppPrivate *priv, xmlNode *node)
{
	xmlNode  *l;
	NodeInfo *info = bonobo_ui_xml_get_data (priv->tree, node);

	if (info->widget)
		container_destroy_children (GTK_CONTAINER (info->widget));
	else {
		guint          dummy;
		const char    *dockname = xmlGetProp (node, "name");
		GnomeDockItem *item;
		GtkWidget     *toolbar;

		item = gnome_dock_get_item_by_name (priv->dock,
						    dockname,
						    &dummy, &dummy,
						    &dummy, &dummy);

		if (!item) {
			item = GNOME_DOCK_ITEM (
				gnome_dock_item_new (
					dockname, GNOME_DOCK_ITEM_BEH_NORMAL));
			gnome_dock_add_item (priv->dock, item,
					     GNOME_DOCK_TOP,
					     1, 0, 0, TRUE);
		}

		toolbar = bonobo_app_toolbar_new ();
		gtk_container_add (GTK_CONTAINER (item),
				   GTK_WIDGET (toolbar));

		info->widget = toolbar;
	}

	for (l = node->childs; l; l = l->next)
		build_toolbar_widget (priv, l);

	gtk_widget_show_all (GTK_WIDGET (priv->dock));
}

typedef enum {
	UI_UPDATE_MENU,
	UI_UPDATE_STATUS,
	UI_UPDATE_DOCKITEM
} UIUpdateType;

static void
seek_dirty (BonoboAppPrivate *priv, xmlNode *node, UIUpdateType type)
{
	BonoboUIXmlData *info;

	info = bonobo_ui_xml_get_data (priv->tree, node);
	if (info->dirty) { /* Rebuild tree from here down */
		
		bonobo_ui_xml_set_dirty (priv->tree, node, FALSE);

		switch (type) {
		case UI_UPDATE_MENU:
			update_menus (priv, node);
			break;
		case UI_UPDATE_DOCKITEM:
			update_dockitem (priv, node);
			break;
		default:
			g_warning ("No status support yet");
			break;
		}
	} else {
		xmlNode *l;

		for (l = node->childs; l; l = l->next)
			seek_dirty (priv, l, type);
	}

	gtk_widget_show_all (GTK_WIDGET (priv->window));
}

static void
update_widgets (BonoboAppPrivate *priv)
{
	xmlNode *node;

	for (node = priv->tree->root->childs; node; node = node->next) {
		if (!node->name)
			continue;

		if (!strcmp (node->name, "menu")) {
			seek_dirty (priv, node, UI_UPDATE_MENU);

		} else if (!strcmp (node->name, "status")) {
			seek_dirty (priv, node, UI_UPDATE_STATUS);

		} else if (!strcmp (node->name, "dockitem")) {
			seek_dirty (priv, node, UI_UPDATE_DOCKITEM);

		} /* else unknown */
	}
	bonobo_ui_xml_dump (priv->tree, priv->tree->root, "Updated widgets");
#ifdef DO_GUI
	gtk_main ();
#endif
}

void
bonobo_app_set_contents (BonoboApp *app,
			 GtkWidget *contents)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->dock != NULL);

	gnome_dock_set_client_area (app->priv->dock, contents);
}

GtkWidget *
bonobo_app_get_contents (BonoboApp *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app->priv != NULL, NULL);
	g_return_val_if_fail (app->priv->dock != NULL, NULL);

	return gnome_dock_get_client_area (app->priv->dock);
}

static void
destroy_priv (BonoboAppPrivate *priv)
{
	gtk_widget_destroy (priv->window);
		
	gtk_object_unref (GTK_OBJECT (priv->tree));

	g_free (priv->name);
	g_free (priv->prefix);

	g_free (priv);
}

static void
bonobo_app_destroy (GtkObject *object)
{
	BonoboApp *app = (BonoboApp *)object;
	
	if (app) {
		if (app->priv)
			destroy_priv (app->priv);
		app->priv = NULL;
	}
	GTK_OBJECT_CLASS (bonobo_app_parent_class)->destroy (object);
}


/**
 * bonobo_app_get_epv:
 */
POA_Bonobo_App__epv *
bonobo_app_get_epv (void)
{
	POA_Bonobo_App__epv *epv;

	epv = g_new0 (POA_Bonobo_App__epv, 1);

/*	epv->set_client_site = impl_Bonobo_App_set_client_site;
	epv->get_client_site = impl_Bonobo_App_get_client_site;
	epv->set_host_name   = impl_Bonobo_App_set_host_name;
	epv->close           = impl_Bonobo_App_close;
	epv->get_verb_list   = impl_Bonobo_App_get_verb_list;
	epv->advise          = impl_Bonobo_App_advise;
	epv->unadvise        = impl_Bonobo_App_unadvise;
	epv->get_misc_status = impl_Bonobo_App_get_misc_status;
	epv->new_view        = impl_Bonobo_App_new_view;
	epv->set_uri         = impl_Bonobo_App_set_uri;
	epv->new_canvas_item = impl_Bonobo_App_new_canvas_item;*/

	return epv;
}

static void
bonobo_app_corba_class_init ()
{
	bonobo_app_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_app_vepv.Bonobo_App_epv = bonobo_app_get_epv ();
}

static void
bonobo_app_class_init (BonoboAppClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_app_parent_class =
		gtk_type_class (bonobo_object_get_type ());

	object_class->destroy = bonobo_app_destroy;

	bonobo_app_corba_class_init ();
}

static BonoboAppPrivate *
construct_priv (const char *app_name,
		const char *title)
{
	BonoboAppPrivate *priv;
	GnomeDockItemBehavior behavior;

	priv = g_new0 (BonoboAppPrivate, 1);

	priv->name   = g_strdup (app_name);
	priv->prefix = g_strconcat ("/", app_name, "/", NULL);

	priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if (title)
		gtk_window_set_title (GTK_WINDOW (priv->window), title);

	priv->dock   = GNOME_DOCK (gnome_dock_new ());
	gtk_container_add (GTK_CONTAINER (priv->window),
			   GTK_WIDGET    (priv->dock));

	behavior = (GNOME_DOCK_ITEM_BEH_EXCLUSIVE
		    | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	if (!gnome_preferences_get_menubar_detachable ())
		behavior |= GNOME_DOCK_ITEM_BEH_LOCKED;

	priv->menu_item = GNOME_DOCK_ITEM (gnome_dock_item_new (
		"menu", behavior));
	priv->menu      = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_container_add (GTK_CONTAINER (priv->menu_item),
			   GTK_WIDGET    (priv->menu));
	gnome_dock_add_item (priv->dock, priv->menu_item,
			     GNOME_DOCK_TOP, 0, 0, 0, TRUE);

	priv->status_item = GNOME_DOCK_ITEM (gnome_dock_item_new (
		"status",
		(GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL |
		 GNOME_DOCK_ITEM_BEH_LOCKED)));
	priv->status      = GTK_BOX (gtk_hbox_new (FALSE, 0));
	gtk_container_add (GTK_CONTAINER (priv->status_item),
			   GTK_WIDGET    (priv->status));
	gnome_dock_add_item (priv->dock, priv->status_item,
			     GNOME_DOCK_BOTTOM, 0, 0, 0, TRUE);

	gtk_widget_show_all (GTK_WIDGET (priv->window));

	priv->tree = bonobo_ui_xml_new (info_compare_fn,
				       info_new_fn,
				       info_free_fn);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "override",
			    (GtkSignalFunc) override_fn, NULL);

	gtk_signal_connect (GTK_OBJECT (priv->tree), "reinstate",
			    (GtkSignalFunc) reinstate_fn, NULL);

	priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (priv->window),
				    priv->accel_group);

	gtk_widget_show_all (GTK_WIDGET (priv->dock));

	return priv;
}

/**
 * bonobo_app_get_type:
 *
 * Returns: The GtkType for the BonoboApp class.
 */
GtkType
bonobo_app_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboApp",
			sizeof (BonoboApp),
			sizeof (BonoboAppClass),
			(GtkClassInitFunc) bonobo_app_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_object_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_app_corba_object_create:
 * @object: The GtkObject that will wrap the CORBA object.
 *
 * Creates an activates the CORBA object that is wrapped
 * by the BonoboObject @object.
 *
 * Returns: An activated object reference to the created object or
 * %CORBA_OBJECT_NIL in case of failure.
 */
Bonobo_App
bonobo_app_corba_object_create (BonoboObject *object)
{
	POA_Bonobo_App *servant;
	CORBA_Environment ev;
	
	servant = (POA_Bonobo_App *)g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_app_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_App__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}
	CORBA_exception_free (&ev);

	return bonobo_object_activate_servant (object, servant);
}

BonoboApp *
bonobo_app_construct (BonoboApp  *app,
		      Bonobo_App  corba_app,
		      const char *app_name,
		      const char *title)
{
	g_return_val_if_fail (app != NULL, NULL);
	g_return_val_if_fail (app_name != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_APP (app), NULL);
	g_return_val_if_fail (corba_app != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (BONOBO_OBJECT (app), corba_app);
	
	app->priv = construct_priv (app_name, title);

	return app;
}

BonoboApp *
bonobo_app_new (const char   *app_name,
		const char   *title)
{
	Bonobo_App corba_app;
	BonoboApp *app;

	app = gtk_type_new (BONOBO_APP_TYPE);

	corba_app = bonobo_app_corba_object_create (BONOBO_OBJECT (app));
	if (corba_app == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (app));
		return NULL;
	}
	
	return bonobo_app_construct (app, corba_app, app_name, title);
}

void
bonobo_app_xml_merge (BonoboApp  *app,
		      const char *path,
		      const char *xml,
		      gpointer    listener)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->tree != NULL);

	bonobo_ui_xml_merge (app->priv->tree, path, xml, listener);
	update_widgets (app->priv);
}

void
bonobo_app_xml_rm (BonoboApp  *app,
		   const char *path,
		   gpointer    by_listener)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->priv != NULL);
	g_return_if_fail (app->priv->tree != NULL);

	bonobo_ui_xml_rm (app->priv->tree, "/", by_listener);
}
