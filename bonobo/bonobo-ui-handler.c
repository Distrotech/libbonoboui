/*
 * GNOME::UIHandler.
 *
 * Author:
 *    Nat Friedman (nat@gnome-support.com)
 */

/*
 * Notes to self:
 *  - make sure this is a workable model for scripting languages
 *  - comment static routines.
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

	ui_handler->top = g_new0 (GnomeUIHandlerTopLevelData, 1);

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

/*
 * Basic object creation/manipulation functions.
 */

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
void
gnome_ui_handler_set_container (GnomeUIHandler *uih, GNOME_UIHandler container)
{
	g_return_if_fail (uih != NULL, FALSE);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_if_fail (container != CORBA_OBJECT_NIL, FALSE);

	uih->container_uih = container;

	return TRUE;
}

/**
 * gnome_ui_handler_add_containee:
 */
void
gnome_ui_handler_add_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_if_fail (uih != NULL, FALSE);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_if_fail (containee != CORBA_OBJECT_NIL, FALSE);

	uih->containee_uihs = g_list_prepend (uih->containee_uihs, containee);

	return TRUE;
}

/**
 * gnome_ui_handler_remove_containee:
 */
void
gnome_ui_handler_remove_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_if_fail (uih != NULL, FALSE);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);
	g_return_if_fail (containee != CORBA_OBJECT_NIL, FALSE);

	uih->containee_uihs = g_list_remove (uih->containee_uihs, containee);

	/*
	 * FIXME: Unregister this containee with the container, and
	 * remove this containee's UI items and associated callback
	 * data.
	 *
	 * If we are the container, then we have to remove this
	 * containee's menu item widgets.
	 */

	return TRUE;
}

/**
 * gnome_ui_handler_set_accelgroup:
 */
void
gnome_ui_handler_set_accelgroup (GnomeUIHandler *uih, GtkAccelGroup *accelgroup)
{
	g_return_if_fail (uih != NULL, FALSE);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih), FALSE);

	uih->top->accelgroup = accelgroup;
}

/**
 * gnome_ui_handler_set_accelgroup:
 */
GtkAccelGroup *
gnome_ui_handler_get_accelgroup (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return uih->top->accelgroup;
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
 * This helper function replaces all instances of "/" with "\/" and
 * replaces "\" with "\\".
 */
static char *
escape_forward_slashes (char *str)
{
	char *p = str;
	char *new, *newp;

	new = g_malloc (strlen (str) * 2 + 1);
	newp = new;

	while (*p != '\0') {
		if (*p == '/') {
			*newp++ = '\\';
			*newp++ = '/';
		} else if (*p == '\\') {
			*newp++ = '\\';
			*newp++ = '\\';
		} else {
			*newp++ = *p;
		}

		p++;
	}

	*newp = '\0';

	final = g_strdup (new);
	g_free (new);
	return new;
}

/*
 * This helper function replaces all instances of "\/" with "/"
 * and all instances of "\\" with "\".
 */
static char *
unescape_forward_slashes (char *str)
{
	char *p = str;
	char *new, *newp;

	new = g_malloc (strlen (str) + 1);
	newp = new;

	while (*p != '\0') {
		if (*p == '\\' && *(p + 1) == '/') {
			*newp++ = '/';
			p++;
		} else if (*p == '\\' && *(p + 1) == '\\') {
			*newp++ = '\\';
			p++;
		} else {
			*newp++ = *p;
		}

		p++;
	}

	*newp = '\0';

	final = g_strdup (new);
	g_free (new);
	return new;
}

/*
 * The path-building routine.
 */
char *
gnome_ui_handler_build_path (char *comp1, ...)
{
	char *path, *path_component;
	va_list args;

	va_start (args, comp1);
	for (path_component = comp1 ;
	     path_component != NULL;
	     path_component = va_arg (args, char *))
	{
		char *old_path = path;
		char *escaped_component = escape_forward_slashes (path_component);

		path = g_strconcat (old_path, "/", escaped_component);
		g_free (old_path);
	}

	return path;
}

/*
 *
 * Menus.
 *
 * This section contains the implementations for the Menu item
 * manipulation class methods and the CORBA menu manipulation
 * interface methods.
 *
 */


/**
 * gnome_ui_handler_set_menubar:
 */
void
gnome_ui_handler_set_menubar (GnomeUIHandler *uih, GtkWidget *menubar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (menubar);
	g_return_if_fail (GTK_IS_MENUBAR (menubar));

	uih->top->managed_menubar = menubar;

	return TRUE;
}

/**
 * gnome_ui_handler_get_menubar:
 */
GtkWidget *
gnome_ui_handler_get_menubar (GnomeUIHandler *uih, GtkWidget *menubar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (menubar);
	g_return_if_fail (GTK_IS_MENUBAR (menubar));

	return uih->top->managed_menubar;
}

static gchar **
tokenize_path (const char *path)
{
	return g_strsplit (path, "/", 0);
}

static gchar *
get_parent_path (const char *path)
{
	char *parent_path;
	char **toks;
	int i;

	g_return_if_fail (path != NULL);

	if (strlen (path) == 0 || ! strcmp (path, "/"))
		g_warning ("get_parent_path: Asked for parent of \"/\"!\n");
	
	toks = tokenize_path (path);

	parent_path = NULL;
	i = 0;
	while (toks [i + 1] != NULL) {
		char *new_path;

		if (parent_path == NULL)
			new_path = g_strdup (toks [i]);
		else
			new_path = g_strconcat (parent_path, "/", toks[i], NULL);

		g_free (parent_path);
		parent_path = new_path;

		i++;
	}

	if (parent_path == NULL || strlen (parent_path) == 0)
		parent_path = g_strdup ("/");

	/*
	 * Free the token array.
	 */
	for (i = 0; toks [i] != NULL; i ++)
		g_free (toks [i]);
	g_free (toks);


	return parent_path;
} 

static void
do_menu_item_path (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	gchar **parent_toks;
	gchar **item_toks;

	parent_toks = tokenize_path (parent_path);

	/*
	 * If there is a path set on the item already, make sure it
	 * matches the parent path.
	 */
	if (item->path != NULL) {
		int i;
		int paths_match = TRUE;
	
		item_toks = tokenize_path (item->path);

		for (i = 0; parent_toks [i] != NULL; i ++) {
			if (item_toks [i] == NULL || strcmp (item_toks [i], parent_toks [i])) {
				paths_match = FALSE;
				break;
			}
		}

		/*
		 * Make sure there is exactly one more element in the
		 * item_toks array.
		 */
		if (paths_match) {
			if (item_toks [i] == NULL || item_toks [i + 1] != NULL)
				paths_match = FALSE;
		}

		if (! paths_match)
			g_warning ("do_menu_item_path: Item path [%s] does not jibe with parent path [%s]!\n",
				   item->path, parent_path);
	}

	/*
	 * Build a path for the item.
	 */
	if (item->path == NULL)
		item->path = g_strconcat (parent_path, "/", item->label, NULL);

	/*
	 * Free the token lists.
	 */
	for (i = 0; item_toks [i] != NULL; i ++)
		g_free (item_toks [i]);
	g_free (item_toks);

	for (i = 0; parent_toks [i] != NULL; i ++)
		g_free (parent_toks [i]);
	g_free (parent_toks);
}

static GtkWidget *
create_menu_gtk_label (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, guint *keyval)
{
	GtkWidget *label;

	/* FIXME: Is this the right way to do the i18n? */
	label = gtk_accel_label_new (gettext (item->label));

	*keyval = gtk_label_parse_uline (GTK_LABEL (label), item->label);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	return label;
}

static GtkWidget *
create_gtk_menu_item (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item, guint *keyval)
{
	GtkWidget *menu_item;

	switch (item->type) {
	case GNOME_UI_HANDLER_MENU_ITEM:
	case GNOME_UI_HANDLER_MENU_SUBTREE:

		/*
		 * Create a GtkMenuItem widget if it's a plain item.  If it contains
		 * a pixmap, create a GtkPixmapMenuItem.
		 */
		if (item->pixmap_data != NULL && item->pixmap_type != GNOME_UI_HANDLER_PIXMAP_NONE) {
			GtkWidget *pixmap;

			menu_item = gtk_pixmap_menu_item_new ();

			pixmap = create_pixmap (uih, item->pixmap_type, item->pixmap_data);
			if (pixmap != NULL) {
				gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_item), pixmap);
				gtk_widget_show (pixmap);
			}
		} else {
			menu_item = gtk_menu_item_new ();
		}

		break;

	case GNOME_UI_HANDLER_MENU_RADIOITEM:
		GSList *radiogroup;

		/*
		 * The parent path should be the path to the radio group GSList.
		 */
		radiogroup = g_hash_table_lookup (uih->top->path_to_menu_item,
						  parent_path);

		/*
		 * Create the new radio menu item and add it
		 * to the radio group.
		 */
		menu_item = gtk_radio_menu_item_new (radiogroup);
		radiogroup = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
		g_hash_table_insert (uih->top->path_to_menu_item, parent_path, radiogroup);

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_TOGGLEITEM:
		menu_item = gtk_check_menu_item_new ();

		/*
		 * Show the checkbox and set its initial state.
		 */
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), FALSE);
		break;

	case GNOME_UI_HANDLER_MENU_SEPARATOR:

		/*
		 * A separator is just a plain menu item with its sensitivity
		 * set to FALSE.
		 */
		menu_item = gtk_menu_item_new ();
		gtk_widget_set_sensitive (menu_item, FALSE);
		break;

	default:
		g_warning ("create_menu_item: Invalid GnomeUIHandlerMenuItemType %d\n",
			   (int) item->type);
		return;
			
	}

	if (uih->accelgroup == NULL)
		gtk_widget_lock_accelerators (menu_item);

	/*
	 * Create a label for the menu item.
	 */
	if (item->label != NULL) {
		GtkWidget *label;

		/* Create the label. */
		label = create_menu_gtk_label (uih, item, keyval);

		/*
		 * Insert it into the menu item widget and setup the
		 * accelerator.
		 */
		gtk_container_add (GTK_CONTAINER (menu_item), label);
		gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menu_item);
	}
	
	gtk_widget_show (menu_item);

	return menu_item;
}

/*
 * This function maps a path to either a subtree menu shell or the
 * top-level menu bar.  It returns NULL if the specified path is
 * not a subtree path (or "/").
 */
static GtkWidget *
get_menu_shell (GnomeUIHandler *uih, char *path)
{
	if (! strcmp (path, "/"))
		return uih->top->menubar;

	return g_hash_table_lookup (uih->top->path_to_menu_shell, path);
}

static gint
save_accels (gpointer data)
{
	gchar *file_name;

	file_name = g_concat_dir_and_file (gnome_user_accels_dir, gnome_app_id);
	gtk_item_factory_dump_rc (file_name, NULL, TRUE);
	g_free (file_name);

	return TRUE;
}

static void
install_global_accelerators (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, GtkWidget *menu_item)
{
	static guint save_accels_id = 0;
	char *globalaccelstr;

	/*
	 * FIXME: I don't know exactly what this does.
	 * FIXME: I'm also not sure if we can guarantee that gnome_app_id isn't NULL here.
	 */
	globalaccelstr = g_strconcat (gnome_app_id, ">", item->path, "<", NULL);
	gtk_item_factory_add_foreign (menu_item, globalaccelstr, uih->top->accelgroup,
				      item->accelerator_key, item->ac_mods);
	g_free (globalaccelstr);

	if (! save_accels_id)
		save_accels_id = gtk_quit_add (1, save_accels, NULL);

}

/*
 * This function is the callback through which all GtkMenuItem
 * activations pass.  If a user selects a menu item, this function is
 * the first to know about it.
 *
 * So the job of this function is to dispatch to the appropriate user
 * callback or containee object.
 */
static gint
menu_item_activated (GtkWidget *menu_item, gpointer data)
{
	char *path = (char *) data;
	GNOME_UIHandler containee;
	GnomeUIHandlerCallbackFunc dispatch_func;

	g_return_val_if_fail (path != NULL, FALSE);

	/*
	 * Check whether this menu item belongs to a containee first,
	 * so that containee menu items override locally-created menu
	 * items.
	 */
	containee = g_hash_table_lookup (uih->menu_path_to_containee, path);
	if (containee != NULL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		GNOME_UIHandler_item_activated (containee, path, &ev);
		/* FIXME: Print an error ? */
		CORBA_exception_free (&ev);

		return FALSE;
	}

	/*
	 * Otherwise, dispatch to a local callback, if one exists.
	 */
	dispatch_func = g_hash_table_lookup (uih->menu_path_to_callback, path);
	if (dispatch_func != NULL) {
		void *callback_closure;

		callback_closure = g_hash_table_lookup (uih->menu_path_to_callback_closure, path);

		(* dispatch_func) (path, callback_closure);
	}

	return FALSE;
}


static void
create_menu_item (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item)
{
	GtkWidget *menu_item;
	GtkWidget *parent_menu_shell;
	guint keyval;

	/*
	 * FIXME: Handle overriding
	 */
		 
	/*
	 * Make sure that the parent path exists before creating the
	 * item.
	 */
	parent_menu_shell = get_menu_shell (parent_path);
	g_return_if_fail (parent_menu_shell != NULL);

	/*
	 * If the provided GnomeUIHandlerMenuItem does not have a path
	 * set, set one for it.
	 */
	do_menu_item_path (uih, parent_path, item);

	/*
	 * Create the GtkMenuItem widget for this item, and stash it
	 * in the hash table.
	 */
	menu_item = create_gtk_menu_item (uih, parent_path, item, &keyval);
	g_hash_table_insert (uih->top->path_to_menu_item, item->path, menu_item);

	/*
	 * Insert the new GtkMenuItem into the menu shell of the
	 * parent.
	 */
	gtk_menu_shell_insert (menu_shell, menu_item, pos);

	/*
	 * If it's a subtree, create the menu shell.
	 */
	if (item->type == GNOME_UI_HANDLER_MENU_SUBTREE) {
		GtkMenu *menu;
		GtkWidget *tearoff;

		/* Create the menu shell. */
		menu = gtk_menu_new ();

		/* Setup the accelgroup for this menu shell. */
		gtk_menu_set_accelgroup (menu, uih->accelgroup);

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
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);

		/*
		 * Store the menu shell for later retrieval (e.g. when
		 * we add submenu items into this shell).
		 */
		g_hash_table_insert (uih->top->path_to_menu_shell, item->path, menu);
	}

	/*
	 * Setup the underline accelerators.
	 *
	 * FIXME: Should this be an option?
	 */
	if (keyval != GDK_VoidSymbol) {
		if (GTK_IS_MENU (menu_shell))
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    gtk_menu_ensure_uline_accel_group (GTK_MENU (menu_shell)),
						    keyval, 0,
						    0);

		if (GTK_IS_MENU_BAR (menu_shell) && uih->accelgroup != NULL)
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    uih->accelgroup,
						    keyval, GDK_MOD1_MASK,
						    0);
	}

	/*
	 * Install the global accelerators.
	 *
	 * FIXME: I'm not entirely sure what this does.
	 */
	install_global_accelerators (uih, item, menu_item);

	/*
	 * Connect to the activate signal.
	 *
	 * FIXME: Think hard.
	 */
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", menu_item_activated, item->path);

	/*
	 * FIXME: We should be duplicating the GnomeUIHandlerMenuItem structure
	 * and storing it internally.
	 */
}

void
gnome_ui_handler_menu_add_one (GnomeUIHandler *uih, char *parent_path,
			       GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * If we are a containee, then we propagate the menu item
	 * construction request to our container.
	 */
	if (uih->container_uih) {
		/* FIXME: propagate */
	} else {
		create_menu_item (uih, parent_path_item);
	}

	/*
	 * Store the callback data.
	 */
	g_hash_table_insert (uih->menu_path_to_callback, path, item->callback);
	g_hash_table_insert (uih->menu_path_to_callback_closure, path, item->callback_data);
}

void
gnome_ui_handler_menu_add_one_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_menu_add_list (GnomeUIHandler *uih, char *parent_path,
			       GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; *curr != NULL; curr ++)
		gnome_ui_handler_menu_add_tree (uih, parent_path, curr);
}

void
gnome_ui_handler_menu_add_list_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_menu_add_tree (GnomeUIHandler *uih, char *parent_path,
				GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_menu_add_tree_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				    GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_menu_remove (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * Remove the callback data for this menu item.
	 */
	g_hash_table_remove (uih->menu_path_to_callback, path);
	g_hash_table_remove (uih->menu_path_to_callback_closure, path);

	/*
	 * FIXME: If this path appears in the path_to_containee list,
	 * then a container is removing its containees menu items.  I
	 * guess this is ok.
	 */
	g_hash_table_remove (uih->menu_path_to_containee, path);

	/*
	 * If we are a containee, then propagate the removal to our
	 * container.  Otherwise, we are the top-level UIHandler, and
	 * should remove the widget manually ourselves.
	 */
	if (uih->container_uih != NULL) {
		/*
		 * FIXME: Propagate the removal.
		 */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		gtk_widget_destroy (menu_item);
	}
}

void
gnome_ui_handler_menu_set_info (GnomeUIHandler *uih, char *path,
				GnomeUIHandlerMenuItem *info)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (info != NULL);
}

GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_fetch (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *item;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

}

GList *
gnome_ui_handler_menu_fetch_by_callback	(GnomeUIHandler *uih,
					 GnomeUIHandlerCallbackFunc callback)
{
	GList *l;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (callback != NULL);

}

GList *
gnome_ui_handler_menu_fetch_by_callback_data (GnomeUIHandler *uih,
					      gpointer callback_data)
{
	GList *l;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
}


GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_list (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
	
}

GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_one (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
	
}

gint
gnome_ui_handler_menu_get_pos (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_menu_set_sensitivity (GnomeUIHandler *uih, char *path,
				       gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method along to the
	 * container.
	 *
	 * Otherwise, we fetch the widget and set its sensitivity
	 * ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		gtk_widget_set_sensitive (menu_item, sensitive);
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * FIXME: We need to maintain a GnomeUIHandlerMenuItem tree for all the widgets
	 * so we can figure this out without consulting widget->saved_state.
	 */
}

void
gnome_ui_handler_menu_set_hidden (GnomeUIHandler *uih, char *path,
				  gboolean hidden)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);


	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		gtk_widget_hide (menu_item);
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_hidden (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_menu_set_label (GnomeUIHandler *uih, char *path,
				 gchar *label_text)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;
		GtkWidget *child;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		child = GTK_BIN (menu_item)->child;
		g_return_if_fail (GTK_IS_LABEL (child));

		gtk_label_set_text (GTK_LABEL (label), label_text)
		/* FIXME: update state! */
	}
}

gchar *
gnome_ui_handler_menu_get_label (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_menu_set_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (data != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;
		GtkWidget *pixmap_widget;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		pixmap_widget = create_pixmap (type, data);
		gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_item),
						 pixmap_widget);
		/* FIXME: update state! */
	}
}

void
gnome_ui_handler_menu_get_pixmap (GnomeUIHandler *uih, char *path,
				  GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (type != NULL);
	g_return_if_fail (data != NULL);
}

gboolean
gnome_ui_handler_menu_set_accel (GnomeUIHandler *uih, char *path,
				 guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);


		/* FIXME: this deal with signals */
		gtk_widget_add_accelerator (menu_item, signal_name /* FIXME: ?? */,
					    uih->top->accelgroup, accelerator_key,
					    ac_mods, accel_flags /* FIXME: ?? */);
					    
		/* FIXME: update state! */
	}
}

void
gnome_ui_handler_menu_get_accel (GnomeUIHandler *uih, char *path,
				 guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (accelerator_key != NULL);
	g_return_if_fail (ac_mods != NULL);

	
}

gboolean
gnome_ui_handler_menu_set_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc callback,
				    gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	g_hash_table_insert (uih->menu_path_to_callback, path, callback);
	g_hash_table_insert (uih->menu_path_to_callback_closure, path, callback_data);
}

void
gnome_ui_handler_menu_get_callback (GnomeUIHandler *uih, char *path,
				    GnomeUIHandlerCallbackFunc *callback,
				    gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (callback_data != NULL);

	*callback = g_hash_table_lookup (uih->menu_path_to_callback, path);
	*callback_data = g_hash_table_lookup (uih->menu_path_to_callback_closure, path);
}

gboolean
gnome_ui_handler_menu_set_toggle_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), state);
					    
		/* FIXME: update state! */
	}
}

void
gnome_ui_handler_menu_get_toggle_state (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

}

void
gnome_ui_handler_menu_set_radio_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);


	/*
	 * If we are a containee, then the actual GtkMenuItem widget
	 * is maintained for us by our container (or our container's
	 * container, or ...), so we forward the method call along to
	 * the container.
	 *
	 * Otherwise, we fetch the widget and hide it ourselves.
	 */
	if (uih->container_uih != NULL) {
		/* FIXME: Propagate. */
	} else {
		GtkWidget *menu_item;

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_item, path);
		g_return_if_fail (menu_item != NULL);

		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), state);
					    
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_radio_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

/*
 * Dynamic menu lists.
 */
gboolean
gnome_ui_handler_dynlist_set_maxsize (GnomeUIHandler *uih, char *path,
				      gint maxsize)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gint
gnome_ui_handler_dynlist_get_maxsize (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

char *
gnome_ui_handler_dynlist_prepend_item (GnomeUIHandler *uih, GnomeUIHandlerItem *dynlist,
				       GnomeUIHandlerItem *dynlist_item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (dynlist != NULL);
	g_return_if_fail (dynlist_item != NULL);
}

char *
gnome_ui_handler_dynlist_append_item (GnomeUIHandler *uih, GnomeUIHandlerItem *dynlist,
				      GnomeUIHandlerItem *dynlist_item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (dynlist != NULL);
	g_return_if_fail (dynlist_item != NULL);
}

/*
 * The Toolbar manipulation routines.
 *
 * The name of the toolbar is the first element in the toolbar path.
 * For example:
 *		"Common/Save"
 *		"Graphics Tools/Erase"
 * 
 * Where both "Common" and "Graphics Tools" are the names of 
 * toolbars.
 * 
 */

gboolean
gnome_ui_handler_set_toolbar (GnomeUIHandler *uih, char *name,
			      GtkWidget *toolbar)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
}

GList *
gnome_ui_handler_get_toolbar_list (GnomeUIHandler *uih)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
}

void
gnome_ui_handler_toolbar_create (GnomeUIHandler *uih, char *name)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (name != NULL);
}

void
gnome_ui_handler_toolbar_add_one (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_one_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_list (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_list_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_tree (GnomeUIHandler *uih, char *path,
			      GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_add_tree_pos (GnomeUIHandler *uih, char *path, int pos,
				  GnomeUIHandlerToolbarItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (item != NULL);
}

void
gnome_ui_handler_toolbar_remove (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_info (GnomeUIHandler *uih, char *path,
				   GnomeUIHandlerToolbarItem *info)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (info != NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_fetch (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback (GnomeUIHandler *uih,
					    GnomeUIHandlerCallbackFunc callback)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
}

GList *
gnome_ui_handler_toolbar_fetch_by_callback_data (GnomeUIHandler *uih,
						 gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
}

gint
gnome_ui_handler_toolbar_get_pos (GnomeUIHandler *uih, char *path)
{
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_one (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
}

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_list (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
}

void
gnome_ui_handler_toolbar_set_sensitivity (GnomeUIHandler *uih, char *path,
					  gboolean sensitive)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_get_sensitivity (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_hidden (GnomeUIHandler *uih, char *path, gboolean hidden)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_get_hidden (GnomeUIHandler *uih, char *path, gboolean hidden)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_label (GnomeUIHandler *uih, char *path,
				    gchar *label)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gchar *
gnome_ui_handler_toolbar_get_label (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType type, gpointer data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_pixmap (GnomeUIHandler *uih, char *path,
				     GnomeUIHandlerPixmapType *type, gpointer *data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_accel (GnomeUIHandler *uih, char *path,
				    guint accelerator_key, GdkModifierType ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_accel (GnomeUIHandler *uih, char *path,
				    guint *accelerator_key, GdkModifierType *ac_mods)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_set_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc callback,
				       gpointer callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_get_callback (GnomeUIHandler *uih, char *path,
				       GnomeUIHandlerCallbackFunc *callback,
				       gpointer *callback_data)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_toggle_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}


void
gnome_ui_handler_toolbar_toggle_set_state (GnomeUIHandler *uih, char *path,
					   gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

gboolean
gnome_ui_handler_toolbar_radio_get_state (GnomeUIHandler *uih, char *path)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

void
gnome_ui_handler_toolbar_radio_set_state (GnomeUIHandler *uih, char *path, gboolean state)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
}

				      
/*
 *
 * UI Groups
 *
 */

gint
gnome_ui_handler_group_create (void)
{
}

void
gnome_ui_handler_group_destroy (gint gid)
{
}

void
gnome_ui_handler_group_add_menu_items (gint gid, char *path1, ...)
{
}

void
gnome_ui_handler_group_add_toolbar_items (gint gid, char *path1, ...)
{
}

GList *
gnome_ui_handler_group_get_members (gint gid)
{
}

void
gnome_ui_handler_group_set_sensitivity (gint gid, gboolean sensitivity)
{
}

void
gnome_ui_handler_group_set_hidden (gint gid, gboolean hidden)
{
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
gnome_ui_handler_menu_add (GnomeUIHandler *uih, char *parent_path, GnomeUIInfo *uiinfo)
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
