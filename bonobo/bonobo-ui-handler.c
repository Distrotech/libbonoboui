/*
 * GNOME::UIHandler.
 *
 * Author:
 *    Nat Friedman (nat@gnome-support.com)
 */

#include <config.h>
#include <bonobo/gnome-main.h>
#include <bonobo/gnome-ui-handler.h>

/* Parent object class in GTK hierarchy */
static GnomeObjectClass *gnome_ui_handler_parent_class;

/* The entry point vectors for the server we provide */
static POA_GNOME_UIHandler__epv gnome_ui_handler_epv;
static POA_GNOME_UIHandler__vepv gnome_ui_handler_vepv;

static CORBA_Object
create_gnome_ui_handler (GnomeObject *object)
{
	POA_GNOME_UIHandler *servant;
	CORBA_Object o;
	
	servant = (POA_GNOME_UIHandler *) g_new0 (GnomeObjectServant, 1);
	servant->vepv = &gnome_ui_handler_vepv;

	POA_GNOME_UIHandler__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION) {
		g_free (servant);
		return CORBA_OBJECT_NIL;
	}

	return gnome_object_activate_servant (object, servant);
}

GnomeUIHandler *
gnome_ui_handler_construct (GnomeUIHandler *ui_handler, GNOME_UIHandler corba_uihandler)
{
	g_return_val_if_fail (ui_handler != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (view), NULL);
	g_return_val_if_fail (corba_uihandler != CORBA_OBJECT_NIL, NULL);

	gnome_object_construct (GNOME_OBJECT (ui_handler), corba_uihandler);

	return ui_handler;
}


static void
gnome_ui_handler_destroy (GtkObject *object)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (object);

	/* FIXME: Fill me in */

	GTK_OBJECT_CLASS (gnome_ui_handler_parent_class)->destroy (object);
}

static void
init_ui_handler_corba_class (void)
{
	/* The entry point vectors for this GNOME::View class */
	gnome_ui_handler_epv.size_allocate = impl_GNOME_UIHandler_size_allocate;
	gnome_ui_handler_epv.set_window = impl_GNOME_UIHandler_set_window;
	gnome_ui_handler_epv.do_verb = impl_GNOME_UIHandler_do_verb;

	/* Setup the vector of epvs */
	gnome_ui_handler_vepv.GNOME_obj_epv = &gnome_obj_epv;
	gnome_ui_handler_vepv.GNOME_UIHandler_epv = &gnome_ui_handler_epv;
}

static void
gnome_ui_handler_class_init (GnomeUIHandlerClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_ui_handler_parent_class = gtk_type_class (gnome_object_get_type ());

	object_class->destroy = gnome_ui_handler_destroy;

	init_ui_handler_corba_class ();
}

static void
gnome_ui_handler_init (GnomeObject *object)
{
}

/**
 * gnome_ui_handler_get_type:
 */
GtkType
gnome_ui_handler_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/UIHandler:1.0",
			sizeof (GnomeView),
			sizeof (GnomeViewClass),
			(GtkClassInitFunc) gnome_ui_handler_class_init,
			(GtkObjectInitFunc) gnome_ui_handler_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

/**
 * gnome_ui_handler_new:
 */
GnomeUIHandler *
gnome_ui_handler_new (void)
{
	GNOME_UIHandler corba_uihandler;
	GnomeUIHandler *uih;
	
	uih = gtk_type_new (gnome_ui_handler_get_type ());

	corba_uihandler = create_gnome_ui_handler (GNOME_OBJECT (uih));
	if (corba_uihandler == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (uih));
		return NULL;
	}
	

	return gnome_ui_handler_construct (uih, corba_uihandler);
}

/**
 * gnome_ui_handler_set_container:
 */
gboolean
gnome_ui_handler_set_container (GnomeUIHandler *uih, GNOME_UIHandler container)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (container != CORBA_OBJECT_NIL, FALSE);

	uih->container_uih = container;

	return TRUE;
}

/**
 * gnome_ui_handler_add_containee:
 */
gboolean
gnome_ui_handler_add_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (containee != CORBA_OBJECT_NIL, FALSE);

	uih->containee_uihs = g_list_prepend (uih->containee_uihs, containee);

	return TRUE;
}

/**
 * gnome_ui_handler_set_accelgroup:
 */
gboolean
gnome_ui_handler_set_accelgroup (GnomeUIHandler *uih, GtkAccelGroup *accelgroup)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);

	uih->top->accelgroup = accelgroup;
}

#if 0
/**
 * gnome_ui_handler_set_proxy:
 */
gboolean
gnome_ui_handler_set_proxy (GnomeUIHandler *uih, gboolean proxy)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);

	/*
	 * We can only proxy if we have a containee to which
	 * to forward UI item activation events.
	 */
	if (proxy && uih->containee_uih == CORBA_OBJECT_NIL) {
		g_warning (_("gnome_ui_handler_set_proxy: Cannot forward events to non-existent containee!"));
		return FALSE;
	}

	uih->proxy = proxy;

	return TRUE;
}
#endif

/*
 *
 * Menus.
 *
 */

/**
 * gnome_ui_handler_set_menubar:
 */
gboolean
gnome_ui_handler_set_menubar (GnomeUIHandler *uih, GtkWidget *menubar)
{
	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (menubar, FALSE);
	g_return_val_if_fail (GTK_IS_MENUBAR (menubar), FALSE);

	uih->top->managed_menubar = menubar;

	return TRUE;
}

static GtkMenuShell *
find_menu_shell (GtkWidget *parent, char *path)
{
	GtkWidget *shell;
	gint pos;

#if 0
	shell = gnome_app_find_menu_pos (parent, path, &pos);
#endif
	/* FIXME */

	return GTK_MENU_SHELL (shell);
}

/*
 * This function is called when a containee notifies us via CORBA to
 * add a new menu.  We have to setup the callback such that the events
 * get forwarded to the containee.
 */
static gboolean
add_menu_for_containee (GnomeUIHandler *uih, ...)
{
}

/*
 * This is the callback function for menu items whose 'activate' event
 * must be forwarded to a containee.
 */
static gint
menu_proxying_callback ()
{
}

/**
 * gnome_ui_handler_menu_add: 
 */
gboolean
gnome_ui_handler_menu_add (GnomeUIHandler *uih, char *path, GnomeUIInfo *uiinfo)
{
	GtkWidget *shell;
	gint pos;

	g_return_val_if_fail (uih != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (uiinfo != NULL, FALSE);

	if (uih->container_uih != CORBA_OBJECT_NIL) {
		/* Pass the add along to the container corba object */

		/*
		 * Add each menu item callback to the list of callbacks.
		 */

		/* Traverse UIInfo tree; build path; bind path to callback. */

		/* Return */
	}

	/*
	 * If there is no container corba server, then we are the
	 * top-level UIHandler object and must add this menu entry to
	 * the menubar ourselves.
	 */

	/*
	 * First check that we have a working reference to the menubar.
	 */
	g_return_val_if_fail (uih->managed_menubar != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_MENUBAR (uih->managed_menubar), FALSE);

	/*
	 * Now retrieve the menu shell which corresponds to this
	 * path's parent.  So if the path is "/Blah", then the shell
	 * is the top-level menu shell.  If the path is "/File/Edit"
	 * then the shell is the "/File" menu shell.
	 */
	shell = find_menu_shell (uih->managed_menubar, path);

	if (shell == NULL)
		return FALSE;

	/*
	 * Now add the new menu items.
	 */
	gnome_app_fill_menu (shell, uiinfo, NULL, uih->accelgroup, TRUE, -1);

	return TRUE;
}

/**
 * gnome_ui_handler_menu_add_pos:
 */
gboolean
gnome_ui_handler_menu_add_pos (GnomeUIHandler *uih, char *path, int pos, GnomeUIInfo *uiinfo)
{
}
