/*
 * bonobo-ui-util.c: Bonobo UI utility functions
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#include <config.h>
#include <ctype.h>
#include <string.h>

#include <gtk/gtkstock.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkiconfactory.h>

#include <libgnome/gnome-program.h>

#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-i18n.h>
#include <bonobo/bonobo-ui-node-private.h>

static gchar *find_pixmap_in_path (const gchar *filename);

static const char write_lut[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static const gint8 read_lut[128] = {
	 -1, -1, -1, -1, -1, -1, -1, -1,		/* 0x00 -> 0x07 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1,		/* 0x10 -> 0x17 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1,		/* 0x20 -> 0x27 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,			/* 0x30 -> 0x37 */
	 8,  9, -1, -1, -1, -1, -1, -1,
	 -1, 10, 11, 12, 13, 14, 15, -1,		/* 0x40 -> 0x47 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1,		/* 0x50 -> 0x57 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, 10, 11, 12, 13, 14, 15, -1,		/* 0x60 -> 0x67 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1, -1,		/* 0x70 -> 0x77 */
	 -1, -1, -1, -1, -1, -1, -1, -1,
};

static inline void
write_byte (char *start, guint8 byte)
{
	start[0] = write_lut[byte >> 4];
	start[1] = write_lut[byte & 15];
}

static inline void
write_four_bytes (char *pos, int value) 
{
	write_byte (pos + 0, value >> 24);
	write_byte (pos + 2, value >> 16);
	write_byte (pos + 4, value >> 8);
	write_byte (pos + 6, value);
}

static void
read_warning (const char *start)
{
	g_warning ("Format error in stream '%c', '%c'", start[0], start[1]);
}

static inline guint8
read_byte (const char *start)
{
	guint8 byte1, byte2;
	gint8 nibble1, nibble2;

	byte1 = start[0];
	byte2 = start[1];

	if (byte1 >= 128 || byte2 >= 128)
		read_warning (start);

	nibble1 = read_lut[byte1];
	nibble2 = read_lut[byte2];

	if (nibble1 < 0 || nibble2 < 0)
		read_warning (start);

	return (nibble1 << 4) + nibble2;
}

static inline const guint32
read_four_bytes (const char *pos)
{
	return ((read_byte (pos) << 24) |
		(read_byte (pos + 2) << 16) |
		(read_byte (pos + 4) << 8) |
		(read_byte (pos + 6)));
}

/**
 * bonobo_ui_util_pixbuf_to_xml:
 * @pixbuf: a GdkPixbuf
 * 
 * Convert a @pixbuf to a string representation suitable
 * for passing as a "pixname" attribute with a pixtype
 * attribute = "pixbuf".
 * 
 * Return value: the stringified pixbuf.
 **/
char *
bonobo_ui_util_pixbuf_to_xml (GdkPixbuf *pixbuf)
{
	char   *xml, *dst, *src;
	int 	size, width, height, row, row_stride, col, byte_width;
	gboolean has_alpha;
			
	g_return_val_if_fail (pixbuf != NULL, NULL);

	width  = gdk_pixbuf_get_width  (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	byte_width = width * (3 + (has_alpha ? 1 : 0));

	size =  4 * 2 * 2 + /* width, height */
		1 + 1 +     /* alpha, terminator */ 
		height * byte_width * 2;
	
	xml = g_malloc (size);
	xml [size - 1] = '\0';

	dst = xml;

	write_four_bytes (dst, gdk_pixbuf_get_width  (pixbuf));
	dst+= 4 * 2;

	write_four_bytes (dst, gdk_pixbuf_get_height (pixbuf));
	dst+= 4 * 2;

	if (has_alpha)
		*dst = 'A';
	else
		*dst = 'N';
	dst++;

	/* Copy over bitmap information */	
	src        = gdk_pixbuf_get_pixels    (pixbuf);
	row_stride = gdk_pixbuf_get_rowstride (pixbuf);
			
	for (row = 0; row < height; row++) {

		for (col = 0; col < byte_width; col++) {
			write_byte (dst, src [col]);
			dst+= 2;
		}

		src += row_stride;
	}

	return xml;
}

/**
 * bonobo_ui_util_xml_to_pixbuf:
 * @xml: a string
 * 
 * This converts a stringified pixbuf in @xml into a GdkPixbuf
 * 
 * Return value: a handed reference to the created GdkPixbuf.
 **/
GdkPixbuf *
bonobo_ui_util_xml_to_pixbuf (const char *xml)
{
	GdkPixbuf 	*pixbuf;
	int 		width, height, byte_width;
	int             length, row_stride, col, row;
	gboolean 	has_alpha;
	guint8         *dst;

	g_return_val_if_fail (xml != NULL, NULL);

	while (*xml && isspace ((unsigned char) (*xml)))
		xml++;

	length = strlen (xml);
	g_return_val_if_fail (length > 4 * 2 * 2 + 1, NULL);

	width = read_four_bytes (xml);
	xml += 4 * 2;
	height = read_four_bytes (xml);
	xml += 4 * 2;

	if (*xml == 'A')
		has_alpha = TRUE;
	else if (*xml == 'N')
		has_alpha = FALSE;
	else {
		g_warning ("Unknown type '%c'", *xml);
		return NULL;
	}
	xml++;

	byte_width = width * (3 + (has_alpha ? 1 : 0));
	g_return_val_if_fail (length >= (byte_width * height * 2 + 4 * 2 * 2 + 1), NULL);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, width, height);

	dst        = gdk_pixbuf_get_pixels    (pixbuf);
	row_stride = gdk_pixbuf_get_rowstride (pixbuf);
	
	for (row = 0; row < height; row++) {

		for (col = 0; col < byte_width; col++) {
			dst [col] = read_byte (xml);
			xml += 2;
		}

		dst += row_stride;
	}

	return pixbuf;
}

static gchar *
find_pixmap_in_path (const gchar *filename)
{
	gchar *file;

	if (filename [0] == '/')
		return g_strdup (filename);

	file = gnome_program_locate_file (gnome_program_get (),
					  GNOME_FILE_DOMAIN_PIXMAP,
					  filename, TRUE, NULL);
	return file;
}

static char *
lookup_stock_compat (const char *id)
{
	GtkStockItem item;
	char *lower = g_ascii_strdown (id, -1);
	char *new_id = g_strconcat ("gtk-", lower, NULL);

	if (gtk_stock_lookup (new_id, &item)) {
		g_free (lower);
		return new_id;
	}

	g_free (new_id);
	new_id = g_strconcat ("gnome-stock-", lower, NULL);

	if (gtk_stock_lookup (new_id, &item)) {
		g_free (lower);
		return new_id;
	}

	g_free (lower);

	/* FIXME: does this catch them all ? */

	return NULL;
}

/**
 * bonobo_ui_util_xml_get_icon_widget:
 * @node: the node
 * @icon_size: the desired size of the icon
 * 
 * This function extracts a pixbuf from the node and returns a GtkWidget
 * containing a display of the pixbuf.
 * 
 * Return value: the widget.
 **/
GtkWidget *
bonobo_ui_util_xml_get_icon_widget (BonoboUINode *node, GtkIconSize icon_size)
{
	char      *type, *text;
	GtkWidget *image = NULL;

	g_return_val_if_fail (node != NULL, NULL);

	if (!(type = bonobo_ui_node_get_attr (node, "pixtype")))
		return NULL;

	if (!(text = bonobo_ui_node_get_attr (node, "pixname"))) {
		bonobo_ui_node_free_string (type);
		return NULL;
	}

	if (!text)
		return NULL;

	if (!strcmp (type, "stock")) {
		GtkStockItem item;

		if (gtk_stock_lookup (text, &item))
			image = gtk_image_new_from_stock (text, icon_size);
		else {
			char *mapped;

			if ((mapped = lookup_stock_compat (text))) {
				
				image = gtk_image_new_from_stock (mapped, icon_size);
				g_free (mapped);
			} else
				g_warning ("Unknown stock icon '%s', stock names all changed in Gtk+ 2.0", text);
		}

	} else if (!strcmp (type, "filename")) {
		char *name = find_pixmap_in_path (text);

		if ((name == NULL) || !g_file_test (name, G_FILE_TEST_EXISTS))
			g_warning ("Could not find GNOME pixmap file %s", text);
		else
			image = gtk_image_new_from_file (name);

		g_free (name);

	} else if (!strcmp (type, "pixbuf")) {
		GdkPixbuf *icon_pixbuf;
		
		/* Get pointer to GdkPixbuf */
		icon_pixbuf = bonobo_ui_util_xml_to_pixbuf (text);
		if (icon_pixbuf) {
			image = gtk_image_new_from_pixbuf (icon_pixbuf);
			gdk_pixbuf_unref (icon_pixbuf);
		}
	} else
		g_warning ("Unknown icon_pixbuf type '%s'", type);

	bonobo_ui_node_free_string (text);
	bonobo_ui_node_free_string (type);

	if (image)
		gtk_widget_show (image);

	return image;
}

/**
 * bonobo_ui_util_xml_set_pixbuf:
 * @node: the node
 * @pixbuf: the pixbuf
 * 
 * Associate @pixbuf with this @node by stringifying it and setting
 * the requisite attributes.
 **/
void
bonobo_ui_util_xml_set_pixbuf (BonoboUINode *node,
			       GdkPixbuf    *pixbuf)
{
	char *data;

	g_return_if_fail (node != NULL);
	g_return_if_fail (pixbuf != NULL);

	bonobo_ui_node_set_attr (node, "pixtype", "pixbuf");
	data = bonobo_ui_util_pixbuf_to_xml (pixbuf);
	bonobo_ui_node_set_attr (node, "pixname", data);
	g_free (data);
}

#ifdef FIXME
static void
free_help_menu_entry (GtkWidget *widget, GnomeHelpMenuEntry *entry)
{
       g_free (entry->name);
       g_free (entry->path);
       g_free (entry);
}

static void
bonobo_help_display_cb (BonoboUIComponent *component,
			gpointer           user_data,
			const char        *cname)
{
	gnome_help_display (component, user_data);
}

/*
 * Cut and paste job so we can overcome gnome-libs brokenness.
 */
static char *
bonobo_help_file_find_file (const char *prefix, const char *app,
			    const char *path)
{
	const GList *language_list;
	GString *buf;
	
	gchar *res= NULL;
	gchar *p, c = 0;
	
	language_list= gnome_i18n_get_language_list ("LC_MESSAGES");
	
	while (!res && language_list) {
		const gchar *lang;
		
		lang = language_list->data;
		
		buf = g_string_new (NULL);
		g_string_sprintf (buf, "%s/gnome/help/%s/%s/%s",
				  prefix, app, lang, path);
		res = g_strdup (buf->str);
		p = strrchr (res, '#');
		if (p) {
			c = *p;
			*p = '\0';
		}
		g_string_free (buf, TRUE);
		
		if (!g_file_exists (res)) {
			g_free (res);
			res = NULL;
		}

		if (c && res) {
			*p = c;
			c = 0;
		}
		
		language_list = language_list->next;
	}
	
	return res;
}
#endif

/**
 * bonobo_ui_util_build_help_menu:
 * @listener: associated component
 * @app_prefix: application prefix
 * @app_name: application name
 * @parent: toplevel node
 * 
 * This routine inserts all the help menu items appropriate for this
 * application as children of the @parent node.
 **/
void
bonobo_ui_util_build_help_menu (BonoboUIComponent *listener,
				const char        *app_prefix,
				const char        *app_name,
				BonoboUINode      *parent)
{
#ifdef FIXME
	char buf [1024];
	char *topic_file;
	FILE *file;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (app_name != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (listener));

	/* Try to open help topics file */
	topic_file = gnome_help_file_find_file ((char *)app_name, "topic.dat");
	
	/* Do something sensible */
	if (!topic_file && app_prefix)
		topic_file = bonobo_help_file_find_file (
			app_prefix, app_name, "topic.dat");

	if (!topic_file || !(file = fopen (topic_file, "rt"))) {
		g_warning ("Could not open help topics file %s for app %s", 
				topic_file ? topic_file : "NULL", app_name);

		g_free (topic_file);
		return;
	}
	g_free (topic_file);
	
	/* Read in the help topics and create menu items for them */
	while (fgets (buf, sizeof (buf), file)) {
		unsigned char *s, *id;
		GnomeHelpMenuEntry *entry;
		BonoboUINode *node;

		/* Format of lines is "help_file_name whitespace* menu_title" */
		for (s = buf; *s && !isspace (*s); s++)
			;

		*s++ = '\0';

		for (; *s && isspace (*s); s++)
			;

		if (s [strlen (s) - 1] == '\n')
			s [strlen (s) - 1] = '\0';

		node = bonobo_ui_node_new ("menuitem");
		/* Try and make something unique */
		id = g_strdup_printf ("Help%s%s", app_name, buf);
		bonobo_ui_node_set_attr (node, "name", id);
		bonobo_ui_node_set_attr (node, "verb", id);
		bonobo_ui_node_set_attr (node, "label", s);

		bonobo_ui_node_add_child (parent, node);

		/* Create help menu entry */
		entry = g_new (GnomeHelpMenuEntry, 1);
		entry->name = g_strdup (app_name);
		entry->path = g_strdup (buf);

		bonobo_ui_component_add_verb (listener, id,
					      bonobo_help_display_cb, entry);

		gtk_signal_connect (GTK_OBJECT (listener), "destroy",
				    (GtkSignalFunc) free_help_menu_entry, 
				    entry);
		g_free (id);
	}

	fclose (file);
#endif
}

/**
 * bonobo_ui_util_get_ui_fname:
 * @component_datadir: the datadir for the component, e.g. /usr/share
 * @file_name: the file name of the xml file.
 * 
 * Builds a path to the xml file that stores the GUI.
 * 
 * Return value: the path to the file that describes the
 * UI or NULL if it is not found.
 **/
char *
bonobo_ui_util_get_ui_fname (const char *component_datadir,
			     const char *file_name)
{
	char *fname, *name;

	if ((file_name [0] == '/' || file_name [0] == '.') &&
	    g_file_test (file_name, G_FILE_TEST_EXISTS))
		return g_strdup (file_name);

	if (component_datadir) {
		fname = g_strdup_printf ("%s/gnome-2.0/ui/%s",
					 component_datadir, file_name);

		if (g_file_test (fname, G_FILE_TEST_EXISTS))
			return fname;
		g_free (fname);
	}

	name = g_strconcat (BONOBO_UIDIR, file_name, NULL);
	if (g_file_test (name, G_FILE_TEST_EXISTS))
		return name;
	g_free (name);

	if (component_datadir) {
		name = g_strconcat (component_datadir, "/", file_name, NULL);
		if (g_file_test (name, G_FILE_TEST_EXISTS))
			return name;
		g_free (name);
	}
	
	return NULL;
}

/**
 * bonobo_ui_util_translate_ui:
 * @node: the node to start at.
 * 
 *  Quest through a tree looking for translatable properties
 * ( those prefixed with an '_' ). Translates the value of the
 * property and removes the leading '_'.
 **/
void
bonobo_ui_util_translate_ui (BonoboUINode *node)
{
        BonoboUINode *l;
	int           i;

	if (!node)
		return;

	for (i = 0; i < node->attrs->len; i++) {
		BonoboUIAttr *a;
		const char   *str;

		a = &g_array_index (node->attrs, BonoboUIAttr, i);

		if (!a->id)
			continue;

		str = g_quark_to_string (a->id);
		if (str [0] == '_') {
			char *encoded;

			a->id = g_quark_from_static_string (str + 1);

			encoded = bonobo_ui_util_encode_str (_(a->value));
			xmlFree (a->value);
			a->value = xmlStrdup (encoded);
			g_free (encoded);
		}
	}

	for (l = node->children; l; l = l->next)
		bonobo_ui_util_translate_ui (l);
}

/**
 * bonobo_ui_util_fixup_help:
 * @component: the UI component
 * @node: the node to search under
 * @app_prefix: the application prefix
 * @app_name: the application name
 * 
 * This searches for 'BuiltMenuItems' placeholders, and then
 * fills them with the application's menu items.
 **/
void
bonobo_ui_util_fixup_help (BonoboUIComponent *component,
			   BonoboUINode      *node,
			   const char        *app_prefix,
			   const char        *app_name)
{
	BonoboUINode *l;
	gboolean build_here = FALSE;

	if (!node)
		return;

	if (bonobo_ui_node_has_name (node, "placeholder")) {
		char *txt;

		if ((txt = bonobo_ui_node_get_attr (node, "name"))) {
			build_here = !strcmp (txt, "BuiltMenuItems");
			bonobo_ui_node_free_string (txt);
		}
	}

	if (build_here) {
		bonobo_ui_util_build_help_menu (
			component, app_prefix, app_name, node);
	}

	for (l = bonobo_ui_node_children (node); l; l = bonobo_ui_node_next (l))
		bonobo_ui_util_fixup_help (component, l, app_prefix, app_name);
}

/**
 * bonobo_ui_util_fixup_icons:
 * @node: the node
 * 
 * This function is used to ensure filename pixbuf attributes are
 * converted to in-line pixbufs on the server side, so that we don't
 * sent a ( possibly invalid ) filename across the wire.
 **/
void
bonobo_ui_util_fixup_icons (BonoboUINode *node)
{
	BonoboUINode *l;
	gboolean fixup_here = FALSE;
	char *txt;

	if (!node)
		return;

	if ((txt = bonobo_ui_node_get_attr (node, "pixtype"))) {
		fixup_here = !strcmp (txt, "filename");
		bonobo_ui_node_free_string (txt);
	}

	if (fixup_here &&
	    ((txt = bonobo_ui_node_get_attr (node, "pixname")))) {
		GdkPixbuf *pixbuf = NULL;

		if (g_path_is_absolute (txt))
			pixbuf = gdk_pixbuf_new_from_file (txt, NULL);
		else {
			gchar *name = find_pixmap_in_path (txt);

			if (name) {
				pixbuf = gdk_pixbuf_new_from_file (name, NULL);
				g_free (name);
			}
		}

		if (pixbuf) {
			gchar *xml = bonobo_ui_util_pixbuf_to_xml (pixbuf);

			bonobo_ui_node_set_attr (node, "pixtype", "pixbuf");
			bonobo_ui_node_set_attr (node, "pixname", xml);
			g_free (xml);
		}

		bonobo_ui_node_free_string (txt);
	}

	for (l = bonobo_ui_node_children (node); l; l = bonobo_ui_node_next (l))
		bonobo_ui_util_fixup_icons (l);
}


/**
 * bonobo_ui_util_new_ui:
 * @component: The component help callback should be on
 * @file_name: Filename of the UI file
 * @app_name: Application name ( for finding help )
 * 
 *  Loads an xml tree from a file, cleans the 
 * doc cruft from its nodes; and translates the nodes.
 * 
 * Return value: The translated tree ready to be merged.
 **/
BonoboUINode *
bonobo_ui_util_new_ui (BonoboUIComponent *component,
		       const char        *file_name,
		       const char        *app_prefix,
		       const char        *app_name)
{
	BonoboUINode *node;

	g_return_val_if_fail (app_name != NULL, NULL);
	g_return_val_if_fail (file_name != NULL, NULL);

        node = bonobo_ui_node_from_file (file_name);

	bonobo_ui_util_translate_ui (node);

	bonobo_ui_util_fixup_help (component, node, app_prefix, app_name);

	bonobo_ui_util_fixup_icons (node);

	return node;
}


typedef struct {
	char *file_name;
	char *app_name;
	char *tree;
} BonoboUINodeCacheEntry;

static guint
node_hash (gconstpointer key)
{
	BonoboUINodeCacheEntry *entry = (BonoboUINodeCacheEntry *)key;

	return g_str_hash (entry->file_name) ^ g_str_hash (entry->app_name);
}

static gint
node_equal (gconstpointer a, gconstpointer b)
{
	BonoboUINodeCacheEntry *entry_a = (BonoboUINodeCacheEntry *)a;
	BonoboUINodeCacheEntry *entry_b = (BonoboUINodeCacheEntry *)b;

	return !strcmp (entry_a->file_name, entry_b->file_name) &&
		!strcmp (entry_a->app_name, entry_b->app_name);
}

static GHashTable *loaded_node_cache = NULL;

static void
free_node_cache_entry (BonoboUINodeCacheEntry *entry)
{
	g_free (entry->file_name);
	g_free (entry->app_name);
	g_free (entry->tree);
	g_free (entry);
}

static void
free_loaded_node_cache (void)
{
	if (loaded_node_cache) {
		g_hash_table_foreach (loaded_node_cache,
				      (GHFunc) free_node_cache_entry,
				      NULL);
		g_hash_table_destroy (loaded_node_cache);
	}
}

/**
 * bonobo_ui_util_set_ui:
 * @component: the component
 * @app_datadir: the application datadir eg. /opt/gnome/share
 * @file_name: the filename of the file to merge relative to the prefix.
 * @app_name: the application name - for help merging
 * 
 * This function loads the UI from the associated file, translates it,
 * fixes up all the menus, ensures pixbuf filenames are resolved to xml
 * and then merges the XML to the remote container - this is the best
 * and most simple entry point for the new UI code.
 **/
void
bonobo_ui_util_set_ui (BonoboUIComponent *component,
		       const char        *app_datadir,
		       const char        *file_name,
		       const char        *app_name)
{
	char                  *fname, *ui;
	BonoboUINodeCacheEntry entry, *cached;

	if (!loaded_node_cache) {
		loaded_node_cache = g_hash_table_new (node_hash,
						      node_equal);
		g_atexit (free_loaded_node_cache);
	}

	if (bonobo_ui_component_get_container (component) == CORBA_OBJECT_NIL) {
		g_warning ("Component must be associated with a container first "
			   "see bonobo_component_set_container");
		return;
	}
	
	fname = bonobo_ui_util_get_ui_fname (app_datadir, file_name);
	if (!fname) {
		g_warning ("Can't find '%s' to load ui from", file_name);
		return;
	}

	entry.file_name = (char *) fname;
	entry.app_name  = (char *) app_name;

	cached = g_hash_table_lookup (loaded_node_cache, &entry);

	if (cached) 
		ui = cached->tree;
	else {
		BonoboUINode *node;

		node = bonobo_ui_util_new_ui (
			component, fname, app_datadir, app_name);

		ui = bonobo_ui_node_to_string (node, TRUE);

		bonobo_ui_node_free (node);
		
		cached = g_new (BonoboUINodeCacheEntry, 1);
		
		cached->file_name = g_strdup (fname);
		cached->app_name  = g_strdup (app_name);
		cached->tree      = ui;
		
		g_hash_table_insert (loaded_node_cache, cached, cached);
	}
	
	if (ui)
		bonobo_ui_component_set (component, "/", ui, NULL);
	
	g_free (fname);
}

/*
 * Evil code, cut and pasted from gtkaccelgroup.c
 * needs de-Soptimizing :-)
 */

#define DELIM_PRE '*'
#define DELIM_PRE_S "*"
#define DELIM_POST '*'
#define DELIM_POST_S "*"

static inline gboolean
is_alt (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'a' || string[1] == 'A') &&
		(string[2] == 'l' || string[2] == 'L') &&
		(string[3] == 't' || string[3] == 'T') &&
		(string[4] == DELIM_POST));
}

static inline gboolean
is_ctl (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'c' || string[1] == 'C') &&
		(string[2] == 't' || string[2] == 'T') &&
		(string[3] == 'l' || string[3] == 'L') &&
		(string[4] == DELIM_POST));
}

static inline gboolean
is_modx (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'm' || string[1] == 'M') &&
		(string[2] == 'o' || string[2] == 'O') &&
		(string[3] == 'd' || string[3] == 'D') &&
		(string[4] >= '1' && string[4] <= '5') &&
		(string[5] == DELIM_POST));
}

static inline gboolean
is_ctrl (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'c' || string[1] == 'C') &&
		(string[2] == 't' || string[2] == 'T') &&
		(string[3] == 'r' || string[3] == 'R') &&
		(string[4] == 'l' || string[4] == 'L') &&
		(string[5] == DELIM_POST));
}

static inline gboolean
is_shft (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 's' || string[1] == 'S') &&
		(string[2] == 'h' || string[2] == 'H') &&
		(string[3] == 'f' || string[3] == 'F') &&
		(string[4] == 't' || string[4] == 'T') &&
		(string[5] == DELIM_POST));
}

static inline gboolean
is_shift (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 's' || string[1] == 'S') &&
		(string[2] == 'h' || string[2] == 'H') &&
		(string[3] == 'i' || string[3] == 'I') &&
		(string[4] == 'f' || string[4] == 'F') &&
		(string[5] == 't' || string[5] == 'T') &&
		(string[6] == DELIM_POST));
}

static inline gboolean
is_control (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'c' || string[1] == 'C') &&
		(string[2] == 'o' || string[2] == 'O') &&
		(string[3] == 'n' || string[3] == 'N') &&
		(string[4] == 't' || string[4] == 'T') &&
		(string[5] == 'r' || string[5] == 'R') &&
		(string[6] == 'o' || string[6] == 'O') &&
		(string[7] == 'l' || string[7] == 'L') &&
		(string[8] == DELIM_POST));
}

static inline gboolean
is_release (const gchar *string)
{
	return ((string[0] == DELIM_PRE) &&
		(string[1] == 'r' || string[1] == 'R') &&
		(string[2] == 'e' || string[2] == 'E') &&
		(string[3] == 'l' || string[3] == 'L') &&
		(string[4] == 'e' || string[4] == 'E') &&
		(string[5] == 'a' || string[5] == 'A') &&
		(string[6] == 's' || string[6] == 'S') &&
		(string[7] == 'e' || string[7] == 'E') &&
		(string[8] == DELIM_POST));
}

/**
 * bonobo_ui_util_accel_parse:
 * @accelerator: the accelerator name
 * @accelerator_key: output of the key
 * @accelerator_mods: output of the mods
 * 
 * This parses the accelerator string and returns the key and mods
 * associated with it - using a similar format to Gtk+ but one which
 * doesn't involve inefficient XML entities and avoids other misc.
 * problems.
 **/
void
bonobo_ui_util_accel_parse (char              *accelerator,
			    guint             *accelerator_key,
			    GdkModifierType   *accelerator_mods)
{
	guint keyval;
	GdkModifierType mods;
	gint len;

	g_return_if_fail (accelerator_key != NULL);
	*accelerator_key = 0;
	g_return_if_fail (accelerator_mods != NULL);
	*accelerator_mods = 0;
	g_return_if_fail (accelerator != NULL);
  
	if (accelerator_key)
		*accelerator_key = 0;
	if (accelerator_mods)
		*accelerator_mods = 0;
  
	keyval = 0;
	mods = 0;
	len = strlen (accelerator);
	while (len)
	{
		if (*accelerator == DELIM_PRE)
		{
			if (len >= 9 && is_release (accelerator))
			{
				accelerator += 9;
				len -= 9;
				mods |= GDK_RELEASE_MASK;
			}
			else if (len >= 9 && is_control (accelerator))
			{
				accelerator += 9;
				len -= 9;
				mods |= GDK_CONTROL_MASK;
			}
			else if (len >= 7 && is_shift (accelerator))
			{
				accelerator += 7;
				len -= 7;
				mods |= GDK_SHIFT_MASK;
			}
			else if (len >= 6 && is_shft (accelerator))
			{
				accelerator += 6;
				len -= 6;
				mods |= GDK_SHIFT_MASK;
			}
			else if (len >= 6 && is_ctrl (accelerator))
			{
				accelerator += 6;
				len -= 6;
				mods |= GDK_CONTROL_MASK;
			}
			else if (len >= 6 && is_modx (accelerator))
			{
				static const guint mod_vals[] = {
					GDK_MOD1_MASK, GDK_MOD2_MASK, GDK_MOD3_MASK,
					GDK_MOD4_MASK, GDK_MOD5_MASK
				};

				len -= 6;
				accelerator += 4;
				mods |= mod_vals[*accelerator - '1'];
				accelerator += 2;
			}
			else if (len >= 5 && is_ctl (accelerator))
			{
				accelerator += 5;
				len -= 5;
				mods |= GDK_CONTROL_MASK;
			}
			else if (len >= 5 && is_alt (accelerator))
			{
				accelerator += 5;
				len -= 5;
				mods |= GDK_MOD1_MASK;
			}
			else
			{
				gchar last_ch;
	      
				last_ch = *accelerator;
				while (last_ch && last_ch != DELIM_POST)
				{
					last_ch = *accelerator;
					accelerator += 1;
					len -= 1;
				}
			}
		}
		else
		{
			keyval = gdk_keyval_from_name (accelerator);
			accelerator += len;
			len -= len;
		}
	}
  
	if (accelerator_key)
		*accelerator_key = gdk_keyval_to_lower (keyval);
	if (accelerator_mods)
		*accelerator_mods = mods;
}

/**
 * bonobo_ui_util_accel_name:
 * @accelerator_key: the key
 * @accelerator_mods: the modifiers
 * 
 * This stringifies an @accelerator_key and some @accelerator_mods
 * it is the converse of bonobo_ui_util_accel_parse
 * 
 * Return value: the stringified representation
 **/
gchar *
bonobo_ui_util_accel_name (guint              accelerator_key,
			   GdkModifierType    accelerator_mods)
{
	static const gchar text_release[] = DELIM_PRE_S "Release" DELIM_POST_S;
	static const gchar text_shift[] = DELIM_PRE_S "Shift" DELIM_POST_S;
	static const gchar text_control[] = DELIM_PRE_S "Control" DELIM_POST_S;
	static const gchar text_mod1[] = DELIM_PRE_S "Alt" DELIM_POST_S;
	static const gchar text_mod2[] = DELIM_PRE_S "Mod2" DELIM_POST_S;
	static const gchar text_mod3[] = DELIM_PRE_S "Mod3" DELIM_POST_S;
	static const gchar text_mod4[] = DELIM_PRE_S "Mod4" DELIM_POST_S;
	static const gchar text_mod5[] = DELIM_PRE_S "Mod5" DELIM_POST_S;
	guint l;
	gchar *keyval_name;
	gchar *accelerator;

	accelerator_mods &= GDK_MODIFIER_MASK;

	keyval_name = gdk_keyval_name (gdk_keyval_to_lower (accelerator_key));
	if (!keyval_name)
		keyval_name = "";

	l = 0;
	if (accelerator_mods & GDK_RELEASE_MASK)
		l += sizeof (text_release) - 1;
	if (accelerator_mods & GDK_SHIFT_MASK)
		l += sizeof (text_shift) - 1;
	if (accelerator_mods & GDK_CONTROL_MASK)
		l += sizeof (text_control) - 1;
	if (accelerator_mods & GDK_MOD1_MASK)
		l += sizeof (text_mod1) - 1;
	if (accelerator_mods & GDK_MOD2_MASK)
		l += sizeof (text_mod2) - 1;
	if (accelerator_mods & GDK_MOD3_MASK)
		l += sizeof (text_mod3) - 1;
	if (accelerator_mods & GDK_MOD4_MASK)
		l += sizeof (text_mod4) - 1;
	if (accelerator_mods & GDK_MOD5_MASK)
		l += sizeof (text_mod5) - 1;
	l += strlen (keyval_name);

	accelerator = g_new (gchar, l + 1);

	l = 0;
	accelerator[l] = 0;
	if (accelerator_mods & GDK_RELEASE_MASK)
	{
		strcpy (accelerator + l, text_release);
		l += sizeof (text_release) - 1;
	}
	if (accelerator_mods & GDK_SHIFT_MASK)
	{
		strcpy (accelerator + l, text_shift);
		l += sizeof (text_shift) - 1;
	}
	if (accelerator_mods & GDK_CONTROL_MASK)
	{
		strcpy (accelerator + l, text_control);
		l += sizeof (text_control) - 1;
	}
	if (accelerator_mods & GDK_MOD1_MASK)
	{
		strcpy (accelerator + l, text_mod1);
		l += sizeof (text_mod1) - 1;
	}
	if (accelerator_mods & GDK_MOD2_MASK)
	{
		strcpy (accelerator + l, text_mod2);
		l += sizeof (text_mod2) - 1;
	}
	if (accelerator_mods & GDK_MOD3_MASK)
	{
		strcpy (accelerator + l, text_mod3);
		l += sizeof (text_mod3) - 1;
	}
	if (accelerator_mods & GDK_MOD4_MASK)
	{
		strcpy (accelerator + l, text_mod4);
		l += sizeof (text_mod4) - 1;
	}
	if (accelerator_mods & GDK_MOD5_MASK)
	{
		strcpy (accelerator + l, text_mod5);
		l += sizeof (text_mod5) - 1;
	}
	strcpy (accelerator + l, keyval_name);

	return accelerator;
}

/**
 * bonobo_ui_util_set_pixbuf:
 * @component: the component
 * @path: the path into the xml tree
 * @pixbuf: the pixbuf
 * 
 * This helper function sets a pixbuf at a certain path into an
 * xml tree.
 **/
void
bonobo_ui_util_set_pixbuf (BonoboUIComponent *component,
			   const char        *path,
			   GdkPixbuf         *pixbuf)
{
	char *parent_path;
	BonoboUINode *node;

	node = bonobo_ui_component_get_tree (component, path, FALSE, NULL);

	g_return_if_fail (node != NULL);

	bonobo_ui_util_xml_set_pixbuf (node, pixbuf);

	parent_path = bonobo_ui_xml_get_parent_path (path);
	bonobo_ui_component_set_tree (component, parent_path, node, NULL);

	bonobo_ui_node_free (node);

	g_free (parent_path);
}

