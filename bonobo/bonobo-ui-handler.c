/*
 * GNOME::UIHandler.
 *
 * Author:
 *    Nat Friedman (nat@gnome-support.com)
 */

/*
 * Notes to self:
 *  - make sure this is a workable model for scripting languages.
 *  - comment static routines.
 *  - overriding needs to be handled properly.
 *  - make sure we handle child lists properly.
 *  - handle menu item positions.
 *  - make sure it's easy to walk the tree.  Note that if you don't take a snapshot, it may
 *    change while you walk it.
 *  - change *item == NULL to *item==END_ENTRY
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
	gnome_ui_handler_vepv.GNOME_Unknown_epv = &gnome_object_epv;
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
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	uih->container_uih = container;

	return TRUE;
}

/**
 * gnome_ui_handler_add_containee:
 */
void
gnome_ui_handler_add_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	uih->containee_uihs = g_list_prepend (uih->containee_uihs, containee);

	return TRUE;
}

typedef struct {
	GnomeUIHandler *uih;
	GNOME_UIHandler containee;
} removal_closure_t;

/*
 * This is a helper function used by gnome_ui_handler_rmeove_containee
 * to remove all of the menu items associated with a given containee.a
 */
gboolean
remove_containee_menu_item (gpointer key, gpointer value, gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	char *path = (char *) key;
	GList *l = (GList *) value;

	MenuItemInternal *remove_me;
	GList *curr;

	do {
		remove_me = NULL;
		for (curr = l; curr != NULL; curr = curr->next) {
			MenuItemInternal *internal = (MenuItemInternal *) curr->data;

			if (internal->uih_corba == closure->containee) {
				remove_me = internal;
				break;
			}
		}

		l = g_list_remove (l, remove_me);
	} while (remove_me != NULL);

	/*
	 * If there are no items left in the list, then remove this
	 * entry from the hash table.
	 */
	if (l == NULL)
		return TRUE;

	/*
	 * Otherwise, just insert the new shortened list into the hash
	 * table.
	 *
	 * FIXME: Handle maintaining subtree child lists.
	 *
	 */
	g_hash_table_insert (closure->uih->path_to_menu_item, (char *) key, l);

	return FALSE;
}

gboolean
remove_containee_toolbar_item (gpointer key, gpointer value, gpointer user_data)
{
	removal_closure_t *closure = (removal_closure_t *) user_data;
	char *path = (char *) key;
	GList *l = (GList *) value;

	ToolbarItemInternal *remove_me;
	GList *curr;

	do {
		remove_me = NULL;
		for (curr = l; curr != NULL; curr = curr->next) {
			ToolbarItemInternal *internal = (ToolbarItemInternal *) curr->data;

			if (internal->uih_corba == closure->containee) {
				remove_me = internal;
				break;
			}
		}

		l = g_list_remove (l, remove_me);
	} while (remove_me != NULL);

	/*
	 * If there are no items left in the list, then remove this
	 * entry from the hash table.
	 */
	if (l == NULL)
		return TRUE;

	/*
	 * Otherwise, just insert the new shortened list into the hash
	 * table.
	 */
	g_hash_table_insert (closure->uih->toolbar_path_to_item, (char *) key, l);

	return FALSE;
}

/**
 * gnome_ui_handler_remove_containee:
 */
void
gnome_ui_handler_remove_containee (GnomeUIHandler *uih, GNOME_UIHandler containee)
{
	removal_closure_t *closure;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (containee != CORBA_OBJECT_NIL);

	uih->containee_uihs = g_list_remove (uih->containee_uihs, containee);

	/*
	 * Create a simple closure to pass some data into the removal
	 * function.
	 */
	closure = g_new0 (removal_closure_t, 1);
	closure->uih = uih;
	closure->containee = containee;

	/*
	 * Remove the menu items associated with this containee.
	 */
	g_hash_table_foreach_remove (uih->path_to_menu_item, remove_containee_menu_item, closure);

	/*
	 * Remove the toolbar items associated with this containee.
	 */
	g_hash_table_foreach_remove (uih->path_to_menu_item, remove_containee_toolbar_item, closure);
}

/**
 * gnome_ui_handler_set_accelgroup:
 */
void
gnome_ui_handler_set_accelgroup (GnomeUIHandler *uih, GtkAccelGroup *accelgroup)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));

	uih->top->accelgroup = accelgroup;
}

/**
 * gnome_ui_handler_get_accelgroup:
 */
GtkAccelGroup *
gnome_ui_handler_get_accelgroup (GnomeUIHandler *uih)
{
	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);

	return uih->top->accelgroup;
}

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
	va_end (args);
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

/*
 * This structure is used to maintain an internal representation of a
 * menu item.
 */
typedef struct {

	/*
	 * The GnomeUIHandler.  FIXME: This is gross.  We need it for
	 * menu_item_activated, though.
	 */
	GnomeUIHandler *uih;

	/*
	 * A copy of the GnomeUIHandlerMenuItem for this menu item.
	 */
	GnomeUIHandlerMenuItem *item;

	/*
	 * If this item is a subtree or a radio group, this list
	 * contains the item's children.  It is a list of path
	 * strings.
	 */
	GList *children;

	/*
	 * The UIHandler CORBA interface which owns this particular
	 * menu item.
	 */
	GNOME_UIHandler uih_corba;

	/*
	 * If this item is a radio group, then this list points to the
	 * MenuItemInternal structures for the members of the group.
	 */
	GSList *radio_items;
} MenuItemInternal;

/*
 * This function grabs the current active menu item associated with a
 * given menu path string.
 */
static MenuItemInternal *
get_menu_item (GnomeUIHandler *uih, char *path)
{
	GList *l;

	l = g_hash_table_lookup (uih->path_to_menu_item, path);

	if (l == NULL)
		return NULL;

	return l->data;
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

/*
 * This function checks to make sure that the path of a given
 * GnomeUIHandlerMenuItem is consistent with the path of its parent.
 * If the item does not have a path, one is created for it.  If the
 * path is not consistent with the parent's path, an error is printed
 */
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

	/*
	 * Create the label, translating the provided text, which is
	 * supposed to be untranslated.  This text gets translated in
	 * the domain of the application.
	 */
	label = gtk_accel_label_new (gettext (item->label));

	/*
	 * Parse the underline accelerators out of the label (such as
	 * "_File").  The returned keyval will be added to the application's
	 * accelerators in create_menu_item.
	 */
	*keyval = gtk_label_parse_uline (GTK_LABEL (label), item->label);

	/*
	 * Setup the widget.
	 */
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
		MenuItemInternal *rg;

		/*
		 * The parent path should be the path to the radio
		 * group lead item.
		 */
		rg = get_menu_item (uih, parent_path);

		/*
		 * Create the new radio menu item and add it to the
		 * radio group.
		 */
		menu_item = gtk_radio_menu_item_new (rg->radio_items);
		rg->radio_items = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));

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

	g_return_if_fail (gnome_app_id != NULL);
	globalaccelstr = g_strconcat (gnome_app_id, ">", item->path, "<", NULL);
	gtk_item_factory_add_foreign (menu_item, globalaccelstr, uih->top->accelgroup,
				      item->accelerator_key, item->ac_mods);
	g_free (globalaccelstr);

	if (! save_accels_id)
		save_accels_id = gtk_quit_add (1, save_accels, NULL);

}

static void
impl_item_activated (PortableServer_Servant servant,
		     CORBA_char *path,
		     CORBA_Environment *ev)
{
	GnomeUIHandler *uih = GNOME_UI_HANDLER (gnome_object_from_servant (servant));
	MenuItemInternal *item;

	/*
	 * Grab the internal data structure which represents the
	 * activated menu item.
	 */
	item = get_menu_item (uih, path);

	if (item == NULL) {
		g_warning ("impl_item_activated: Cannot find menu item with path [%s]\n",
			   path);
		return;
	}

	/*
	 * Now call the activation signal handler which will handle
	 * dispatching to the appropriate callback or containee.
	 */
	menu_item_activated (NULL, item);
}

/*
 * This function only applies to top-level UIHandler objects, i.e. the
 * UIHandlers which actually maintain the widgets.
 *
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
	MenuItemInternal *internal = (MenuItemInternal *) data;

	/*
	 * If this item is owned by a containee, then we forward the
	 * activation on to the containee.
	 */
	if (internal->uih_corba != NULL) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		GNOME_UIHandler_item_activated (internal->uih_corba, internal->item->path, &ev);
		/* FIXME: Check exception. */
		CORBA_exception_free (&ev);

		return FALSE;
	}

	/*
	 * If this menu item is locally owned, then we dispatch to the
	 * local callback, if it exists.
	 */
	if (internal->item->callback == NULL)
		return FALSE;

	(internal->item->callback) (internal->uih, internal->item->path, internal->item->callback_data);

	return FALSE;
}

static GnomeUIHandlerMenuItem *
copy_menu_item (GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *copy;	

	copy = g_new0 (GnomeUIHandlerMenuItem, 1);

#define COPY_STRING (x) ((x) == NULL ? NULL : g_strdup (x))
	copy->path = COPY_STRING (item->path);
	copy->type = item->type;
	copy->hint = COPY_STRING (item->hint);
	copy->label = COPY_STRING (item->label);
	copy->children = NULL; /* FIXME: Make sure this gets handled properly. */
	copy->dynlist_maxsize = item->dynlist_maxsize;

	copy->pixmap_data = copy_pixmap_data (item->pixmap_type, item->pixmap_data);
	copy->pixmap_type = item->pixmap_type;

	copy->accelerator_key = item->accelerator_key;
	copy->ac_mods = item->ac_mods;
	copy->callback = item->callback;
	copy->callback_data = item->callback_data;

	return copy;
}

static MenuItemInternal *
store_menu_item_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal, *parent;
	char *parent_path;
	GList *l;

	/*
	 * Create the internal representation of the menu item.
	 */
	internal = g_new0 (MenuItemInternal, 1);
	internal->item = copy_menu_item (item);
	internal->uih = uih;
	internal->uih_corba = uih_corba;
	
	/*
	 * Place this new menu item structure at the front of the linked
	 * list of menu items.
	 *
	 * We maintain a linked list so that we can handle overriding
	 * properly.  The front-most item in the list is the current
	 * active menu item.  When it is destroyed, it is removed from
	 * the list, and the next item in the list is the current,
	 * active menu item.
	 */
	l = g_hash_table_lookup (uih->path_to_menu_item, internal);
	l = g_list_prepend (l, internal);

	g_hash_table_insert (uih->path_to_menu_item, internal->item->path, l);

	/*
	 * Now insert a child record for this item's parent, if it has
	 * a parent.
	 *
	 * FIXME: How do we handle '/' ?
	 *
	 */
	parent_path = get_parent_path (item->path);
	parent = get_menu_item (uih, parent_path);
	g_free (parent_path);

	if (! g_list_find_custom (parent->children, item->path, g_str_equal))
		parent->children = g_list_prepend (parent->children, item->path);

	return internal;
}

/*
 * This internal function does all the work associated with
 * creating a menu item except:
 *
 *	1. Storing the internal representation for the menu item.
 *	2. Connecting to the menu item activation signal.
 */
static GtkWidget *
create_menu_item_widgets (GnomeUIHandler *uih, char *parent_path, GnomeUIHandlerMenuItem *item, int pos)
{
	MenuItemInternal *internal_data;
	GtkWidget *parent_menu_shell;
	GtkWidget *menu_item;
	guint keyval;

	/*
	 * Make sure that the parent path exists before creating the
	 * item.
	 */
	parent_menu_shell = get_menu_shell (parent_path);
	g_return_if_fail (parent_menu_shell != NULL);

	/*
	 * Create the GtkMenuItem widget for this item, and stash it
	 * in the hash table.
	 */
	menu_item = create_gtk_menu_item (uih, parent_path, item, &keyval);
	g_hash_table_insert (uih->top->path_to_menu_widget, item->path, menu_item);

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

	return menu_item;
}


static void
menu_item_connect_signal (GtkWidget *menu_item, MenuItemInternal *internal)
{
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", menu_item_activated, internal_data);
}

/*
 * This function uses create_menu_item_widgets to do most of the work to create
 * a new menu item, and then:
 *
 *	1. Stores the MenuItemInternal structure for the item.
 *	2. Connects to the menu item activation signal.
 */
static void
create_menu_item (GnomeUIHandler *uih, char *parent_path, MenuItemInternal *internal, int pos)
{
	GtkWidget *menu_item;

	/*
	 * This function performs most of the work involved in creating
	 * a new menu item.
	 */
	menu_item = create_menu_item_widgets (uih, parent_path, internal->item, pos);

	/*
	 * Connect to the activate signal.
	 */
	menu_item_connect_signal (menu_item, internal);
}

GNOME_UIHandler_PixmapType
pixmap_type_to_corba (GnomeUIHandlerPixmapType type)
{
	/* FIXME */
}

GnomeUIHandlerPixmapType
corba_to_pixmap_type (GNOME_UIHandler_PixmapType type)
{
	/* FIXME */
}


GNOME_UIHandler_ModifierType
modifier_type_to_corba (GdkModifierType type)
{
	/* FIXME */
}

GdkModifierType
corba_to_modifier_type (GNOME_UIHandler_ModifierType type)
{
	/* FIXME */
}

GNOME_UIHandler_MenuType
menu_type_to_corba (GnomeUIHandlerMenuItemType type)
{
	/* FIXME */
}

GnomeUIHandlerMenuItemType
corba_to_menu_type (GNOME_UIHandler_MenuType type)
{
	/* FIXME */
}

static void
menu_item_container_create (GnomeUIHandler *uih, GnomeUIHandlerMenuItem *item, int pos)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	GNOME_UIHandler_menu_item_create (uih->container_uih,
					  item->path,
					  menu_type_to_corba (item->type),
					  item->label,
					  pixmap_type_to_corba (item->pixmap_type),
					  item->accelerator_key,
					  modifier_type_to_corba (item->modifier),
					  pos);

	/* FIXME: Check exception here */

	CORBA_exception_free (&ev);
}

void
gnome_ui_handler_menu_add_one (GnomeUIHandler *uih, char *parent_path,
			       GnomeUIHandlerMenuItem *item)
{
	MenuItemInternal *internal;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, -1, item);
}

void
gnome_ui_handler_menu_add_one_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	/*
	 * If the provided GnomeUIHandlerMenuItem does not have a path
	 * set, set one for it.
	 */
	do_menu_item_path (uih, parent_path, item);

	/*
	 * Duplicate the GnomeUIHandlerMenuItem structure and store
	 * it internally.
	 *
	 * FIXME: Should we check to make sure there isn't another
	 * menu item for this GNOME_UIHandler ?
	 */
	internal_data = store_menu_item_data (uih, gnome_object_getref (uih), item);

	/*
	 * If we are a containee, then we propagate the menu item
	 * construction request to our container.
	 */
	if (uih->container_uih) {
		menu_item_container_create (uih, item, pos);
	} else {
		create_menu_item (uih, parent_path, internal, pos);
	}
}

/**
 * gnome_ui_handler_menu_add_list:
 */
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

/**
 * gnome_ui_handler_menu_add_list_pos:
 */
void
gnome_ui_handler_menu_add_list_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				   GnomeUIHandlerMenuItem *item)
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	for (curr = item; *curr != NULL; curr ++, pos ++)
		gnome_ui_handler_menu_add_tree_pos (uih, parent_path, pos, curr);
}

/**
 * gnome_ui_handler_menu_add_tree:
 */
void
gnome_ui_handler_menu_add_tree (GnomeUIHandler *uih, char *parent_path,
				GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	gnome_ui_handler_menu_add_tree_pos (uih, parent_path, -1, item);
}

/**
 * gnome_ui_handler_menu_add_tree_pos:
 */
void
gnome_ui_handler_menu_add_tree_pos (GnomeUIHandler *uih, char *parent_path, int pos,
				    GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (parent_path != NULL);
	g_return_if_fail (item != NULL);

	gnome_ui_handler_menu_add_one_pos (uih, parent_path, pos, item);

	if (item->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || item->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		GnomeUIHandlerMenuItem *curr;

		for (curr = item->children; *curr != NULL; curr ++)
			gnome_ui_handler_menu_add_tree (uih, item->path, curr);
	}
}

/**
 * gnome_ui_handler_menu_new_item:
 */
void
gnome_ui_handler_menu_new_item (GnomeUIHandler *uih, char *path,
				GnomeUIHandlerMenuItemType type,
				char *label, char *hint, gint dynlist_maxsize,
				GnomeUIHandlerPixmapType pixmap_type,
				gpointer pixmap_data,
				guint accelerator_key, GdkModifierType ac_mods,
				GnomeUIHandlerCallbackFunc callback,
				gpointer callback_data,
				int pos)
{
	GnomeUIHandlerMenuItem *item;
	char *parent_path;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	parent_path = get_parent_path (path);
	g_return_if_fail (parent_path != NULL);

	item = g_new0 (GnomeUIHandlerMenuItem, 1);
	item->path = path;
	item->type = type;
	item->label = label;
	item->hint = hint;
	item->dynlist_maxsize = dynlist_maxsize;
	item->pixmap_type = pixmap_type;
	item->pixmap_data = pixmap_data;
	item->accelerator_key = accelerator_key;
	item->ac_mods = ac_mods;
	item->callback = callback;
	item->callback_data = callback_data;

	gnome_ui_handler_add_one_pos (uih, parent_path, -1, item);

	g_free (item);
	g_free (parent_path);
}

/*
 * This function removes a menu item from its parent's
 * child list.
 */
static void
remove_parent_menu_entry (GnomeUIHandler *uih, char *path)
{
	MenuItemInternal *parent;
	char *parent_path;
	GList *l;

	parent_path = get_parent_path (path);
	parent = get_menu_item (uih, parent_path);
	g_free (parent_path);

	l = g_list_find_custom (parent->children, path, g_str_equal);
	parent->children = g_list_remove_link (parent->children, l);
}

static void
free_menu_item (GnomeUIHandlerMenuItem *item)
{
	g_free (item->path);
	g_free (item->label);
	g_free (item->hint);
	g_free (item->children);
	free_pixmap_data (item->pixmap_data);

	g_free (item);
}

static gboolean
remove_menu_item_data (GnomeUIHandler *uih, GNOME_UIHandler uih_corba, char *path)
{
	MenuItemInternal *remove_me;
	gboolean head_removed;
	GList *l, *curr;

	/*
	 * Get the list of menu items which match this path.  There
	 * may be several menu items matching the same path because
	 * each containee can create a menu item with the same path.
	 * The newest item overrides the older ones.
	 */
	l = g_hash_table_lookup (uih->path_to_menu_item, path);

	remove_me = NULL;
	for (curr = l; curr != NULL; curr = curr->next) {
		MenuItemInternal *internal = (MenuItemInternal *) l->data;

		if (internal->uih_corba == uih_corba) {
			/*
			 * This is a match.  Remove it.
			 */
			remove_me = internal;
			break;
		}
	}

	if (remove_me == l->data)
		head_removed = TRUE;
	else
		head_removed = FALSE;

	if (remove_me == NULL) {
		g_warning ("remove_menu_item_data: Menu item [%s] matching "
			   "UIHandler CORBA interface [%d] not found!\n",
			   path, GPOINTER_TO_INT (uih_corba));
		return head_removed;
	}

	/*
	 * Free the internal data structures associated with this
	 * item.
	 */
	free_menu_item (internal->item);
	g_free (internal);
	l = g_list_remove (l, internal);

	if (l != NULL) {
		g_hash_table_insert (uih->path_to_menu_item, path);
	} else {
		g_hash_table_remove (uih->path_to_menu_item, path);

		/*
		 * Remove this path entry from the parent's child
		 * list.
		 */
		remove_parent_menu_entry (uih, path);
	}

	return head_removed;
}

/**
 * gnome_ui_handler_menu_remove:
 */
void
gnome_ui_handler_menu_remove (GnomeUIHandler *uih, char *path)
{
	gboolean head_removed;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * Remove the internal data structures associated with this
	 * menu item.
	 */
	head_removed = remove_menu_item_data (uih, gnome_object_getref (uih), path);

	/*
	 * If we are the top-level UIHandler (which is the same as
	 * having no container), then we just remove the widget
	 * ourselves.
	 */
	if (uih->container_uih == NULL) {
		GtkWidget *menu_item;
		MenuItemInternal *internal;

		if (head_removed) {

			/*
			 * Destroy the old menu item widget.
			 */
			menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
			g_return_if_fail (menu_item != NULL);

			gtk_widget_destroy (menu_item);

			g_hash_table_remove (uih->top->path_to_menu_widget, path);

			/*
			 * Create its replacement.
			 */
			internal = get_menu_item (uih, path);
			if (internal != NULL) {
				char *parent_path = get_parent_path (path);

				menu_item = create_gtk_menu_item (uih, parent_path, internal->item);
				g_free (parent_path);

				menu_item_connect_signal (menu_item, internal);
			}
		}

	} else {

		/*
		 * If we are a containee, then we need to propagate
		 * the destruction to our container.  Then, if we have
		 * another menu item which has the same path, we need
		 * to make that menu item replace the deleted one.  So
		 * we tell our container to create the replacement.
		 */
		CORBA_Environment ev;
		GList *l;

		if (head_removed) {
			/*
			 * Tell the container to destroy the menu item.
			 */
			CORBA_exception_init (&ev);
			GNOME_UIHandler_menu_item_remove (uih->container_uih, path, &ev);
			/* FIXME: Check the exception. */
			CORBA_exception_free (&ev);

			/*
			 * Now tell the container to create the replacement
			 * menu item, if one exists.  If one doesn't exist,
			 * then we are done.
			 */
			l = g_hash_table_lookup (uih->path_to_menu_item, path);
			if (l->data == NULL)
				return;

			menu_item_container_create (uih, (GnomeUIHandlerMenuItem *) l->data);
		}
	}
}

/**
 * gnome_ui_Handler_menu_set_info:
 */
void
gnome_ui_handler_menu_set_info (GnomeUIHandler *uih, char *path,
				GnomeUIHandlerMenuItem *info)
{
	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);
	g_return_if_fail (info != NULL);

	gnome_ui_handler_menu_remove (uih, path);
	gnome_ui_handler_menu_add (uih, path, info);
}

static GnomeUIHandlerMenuItem *
get_container_menu_item (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *item;
	CORBA_Environment ev;

	GNOME_UIHandler_PixmapType corba_pixmap_type;
	GNOME_UIHandler_ModifierType corba_modifier_type;
	GNOME_UIHandler_MenuType corba_menu_type;
	GNOME_UIHandler_iobuf corba_pixmap_iobuf;
	CORBA_unsigned_long corba_accelerator_key;
	CORBA_long corba_pos;

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	CORBA_exception_init (&ev);
	GNOME_UIHandler_menu_item_fetch (path, &corba_menu_type, &item->label,
					 &corba_pixmap_type, &corba_pixmap_iobuf,
					 &corba_accelerator_key, &corba_modifier_type,
					 &corba_pos);

	item->path = g_strdup (path);
	item->type = corba_to_menu_type (corba_menu_type);
	item->pixmap_type = corba_to_pixmap_type (corba_pixmap_type);
	item->pixmap_data = g_memdup (iobuf._buffer, iobuf._length);
	item->accelerator_key = accelerator_key;
	item->modifier = corba_to_modifier_type (corba_modifier_type);
	/* FIXME: pos? */

	return item;
}

/**
 * gnome_ui_handler_menu_fetch_one:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_fetch_one (GnomeUIHandler *uih, char *path)
{
	GnomeUIHandlerMenuItem *item;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (path != NULL);

	/*
	 * Check if we have a local copy of this menu item.
	 */
	item = get_menu_item (uih, path);
	if (item != NULL)
		return copy_menu_item (item);

	/*
	 * If we don't, check with our container.
	 */
	if (uih->container_uih == NULL)
		return NULL;

	return get_container_menu_item (uih, path);
}

void
gnome_ui_handler_menu_free_one (GnomeUIHandlerMenuItem *item)
{
	g_return_if_fail (item != NULL);

	free_menu_item (item);
}

void
gnome_ui_handler_menu_free_list (GnomeUIHandlerMenuItem *array);
{
	GnomeUIHandlerMenuItem *curr;

	g_return_if_fail (array != NULL);

	for (curr = array; *curr != NULL; curr ++)
		free_menu_item (curr);
}

void
gnome_ui_handler_menu_free_tree (GnomeUIHandlerMenuItem *tree)
{
	g_return_if_fail (tree != NULL);

	if (*tree == NULL)
		return;

	if (tree->type == GNOME_UI_HANDLER_MENU_SUBTREE
	    || tree->type == GNOME_UI_HANDLER_MENU_RADIOGROUP) {
		GnomeUIHandlerMenuItem *curr_child;

		for (curr_child = tree->children; curr_child != NULL; curr_child ++)
			gnome_ui_handler_menu_free_tree (curr_child);

		g_free (tree->children);
	}

	free_menu_item (tree);
}

typedef struct {
	GnomeUIHandlerCallbackFunc callback;
	GList *l;
} find_callback_closure_t;

static gboolean
find_callback (gpointer key, gpointer val, gpointer data)
{
	MenuItemInternal *item = (MenuItemInternal *) val;
	find_callback_closure_t *closure = (find_callback_closure_t *) data;

	if (item->item->callback == closure->callback)
		closure->l = g_list_prepend (closure->l, item->item);

	return FALSE;
}

GList *
gnome_ui_handler_menu_fetch_by_callback	(GnomeUIHandler *uih,
					 GnomeUIHandlerCallbackFunc callback)
{
	find_callback_closure_t *closure;
	GList *l;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (callback != NULL);

	closure = g_new0 (find_callback_closure_t, 1);
	closure->callback = callback;

	g_hash_table_foreach (uih->path_to_menu_item, find_callback, &l);

	l = closure->l;
	g_free (closure);

	return l;
}


typedef struct {
	gpointer data;
	GList *l;
} find_callback_data_closure_t;

static gboolean
find_callback_data (gpointer key, gpointer val, gpointer data)
{
	MenuItemInternal *item = (MenuItemInternal *) val;
	find_callback_closure_t *closure = (find_callback_closure_t *) data;

	if (item->item->callback_data == closure->data)
		closure->l = g_list_prepend (closure->l, item->item);

	return FALSE;
}

GList *
gnome_ui_handler_menu_fetch_by_callback_data (GnomeUIHandler *uih,
					      gpointer callback_data)
{
	find_callback_data_closure_t *closure;
	GList *l;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (callback != NULL);

	closure = g_new0 (find_callback_data_closure_t, 1);
	closure->callback = callback;

	g_hash_table_foreach (uih->path_to_menu_item, find_callback_data, &l);

	l = closure->l;
	g_free (closure);

	return l;
}

/**
 * gnome_ui_handler_menu_get_child_paths:
 */
GList *
gnome_ui_handler_menu_get_child_paths (GnomeUIHandler *uih, char *parent_path)
{
	MenuItemInternal *parent;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (parent_path != NULL, NULL);

	parent = menu_get_item (parent_path);
	g_return_vail_if_fail (parent != NULL, NULL);

	return g_list_copy (parent->children);
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_one:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_one (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);

	item = g_new0 (GnomeUIHandlerMenuItem, 1);

	
	
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_list:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_list (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
	
}

/**
 * gnome_ui_handler_menu_parse_uiinfo_tree:
 */
GnomeUIHandlerMenuItem *
gnome_ui_handler_menu_parse_uiinfo_tree (GnomeUIHandler *uih, GnomeUIInfo *uii)
{
	GnomeUIHandlerMenuItem *item_tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (GNOME_IS_UI_HANDLER (uih));
	g_return_if_fail (uii != NULL);
	
}

/**
 * gnome_ui_handler_menu_get_pos:
 */
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		pixmap_widget = create_pixmap (type, data);
		gtk_pixmap_menu_item_set_pixmap (GTK_PIXMAP_MENU_ITEM (menu_item),
						 pixmap_widget);
		/* FIXME: update internal astate! */
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
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

		menu_item = g_hash_table_lookup (uih->top->path_to_menu_widget, path);
		g_return_if_fail (menu_item != NULL);

		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), state);
					    
		/* FIXME: update state! */
	}
}

gboolean
gnome_ui_handler_menu_get_radio_state (GnomeUIHandler *uih, char *path)
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

GnomeUIHandlerToolbarItem *
gnome_ui_handler_toolbar_parse_uiinfo_tree (GnomeUIHandler *uih, GnomeUIInfo *uii)
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
