#include "config.h"
#include <gnome.h>
#include <ctype.h>

#include "bonobo-ui-util.h"

static void
write_byte (char *start, guint8 byte)
{
	int chunk;

	chunk = (byte >> 4) & 0xf;

	if (chunk < 10)
		*start++ = '0' + chunk;
	else
		*start++ = 'a' + chunk - 10;

	chunk = byte & 0xf;

	if (chunk < 10)
		*start = '0' + chunk;
	else
		*start = 'a' + chunk - 10;
}

static char *
write_four_bytes (char *pos, int value) 
{
	write_byte (pos, value >> 24);
	pos += 2;
	write_byte (pos, value >> 16);
	pos += 2;
	write_byte (pos, value >> 8);
	pos += 2;
	write_byte (pos, value);
	pos += 2;

	return pos;
}

static guint8
read_byte (const char *start)
{
	int chunk = 0;

	if (*start >= '0' &&
	    *start <= '9')
		chunk |= *start - '0';

	else if (*start >= 'a' &&
		 *start <= 'f')
		chunk |= *start - 'a' + 10;

	else if (*start >= 'A' &&
		 *start <= 'F')
		chunk |= *start - 'A' + 10;
	else
		g_warning ("Format error in stream '%c'", *start);

	chunk <<= 4;
	start++;

	if (*start >= '0' &&
	    *start <= '9')
		chunk |= *start - '0';

	else if (*start >= 'a' &&
		 *start <= 'f')
		chunk |= *start - 'a';

	else if (*start >= 'A' &&
		 *start <= 'F')
		chunk |= *start - 'A';
	else
		g_warning ("Format error in stream '%c'", *start);

	return chunk;
}

static const guint32
read_four_bytes (const char *pos)
{
	return ((read_byte (pos) << 24) |
		(read_byte (pos + 2) << 16) |
		(read_byte (pos + 4) << 8) |
		(read_byte (pos + 6)));
}

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
	xml [size] = '\0';

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

GdkPixbuf *
bonobo_ui_util_xml_to_pixbuf (const char *xml)
{
	GdkPixbuf 	*pixbuf;
	int 		width, height, byte_width;
	int             length, row_stride, col, row;
	gboolean 	has_alpha;
	guint8         *dst;

	g_return_val_if_fail (xml != NULL, NULL);

	while (*xml && isspace (*xml))
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

#define ALPHA_THRESHOLD 128

static GtkWidget *
gnome_pixmap_new_from_pixbuf (GdkPixbuf *pixbuf)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail (pixbuf != NULL, NULL);
	
	gpixmap = gtk_type_new (gnome_pixmap_get_type ());

	/* Get GdkPixmap and mask */
	gdk_pixbuf_render_pixmap_and_mask (
		pixbuf, &gpixmap->pixmap,
		&gpixmap->mask, ALPHA_THRESHOLD);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
bonobo_ui_util_xml_get_pixmap (GtkWidget *window, xmlNode *node)
{
	GtkWidget *pixmap = NULL;
	char      *type;

	g_return_val_if_fail (node != NULL, NULL);

	if (! (type = xmlGetProp (node, "pixtype")))
		return NULL;

	if (!strcmp (type, "stock")) {
		char      *text;

		text = xmlGetProp (node, "pixname");
		pixmap = gnome_stock_pixmap_widget (window, text);

	} else if (!strcmp (type, "filename")) {
		char *name, *text;

		text = xmlGetProp (node, "pixname");

		name = gnome_pixmap_file (text);

		if (name == NULL)
			g_warning ("Could not find GNOME pixmap file %s", text);
		else
			pixmap = gnome_pixmap_new_from_file (name);

		g_free (name);

	} else if (!strcmp (type, "pixbuf")) {
		char      *text;
		GdkPixbuf *pixbuf;

		text = xmlNodeGetContent (node);

		g_return_val_if_fail (text != NULL, NULL);
		
		/* Get pointer to GdkPixbuf */
		pixbuf = bonobo_ui_util_xml_to_pixbuf (text);
		g_return_val_if_fail (pixbuf != NULL, NULL);

		pixmap = gnome_pixmap_new_from_pixbuf (pixbuf);

		gdk_pixbuf_unref (pixbuf);
	} else
		g_warning ("Unknown pixmap type '%s'", type);

	return pixmap;
}

void
bonobo_ui_util_xml_set_pixbuf (xmlNode     *node,
				GdkPixbuf   *pixbuf)
{
	char *data;

	g_return_if_fail (node != NULL);
	g_return_if_fail (pixbuf != NULL);

	xmlSetProp    (node, "pixtype", "pixbuf");
	data = bonobo_ui_util_pixbuf_to_xml (pixbuf);
	xmlNodeSetContent (node, data);
	g_free (data);
}

void
bonobo_ui_util_xml_set_xpm (xmlNode     *node,
			     const char **xpm)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (xpm != NULL);
	g_return_if_fail (node != NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm);

	bonobo_ui_util_xml_set_pixbuf (node, pixbuf);

	gdk_pixbuf_unref (pixbuf);
}
				     
void
bonobo_ui_util_xml_set_stock (xmlNode     *node,
			      const char  *name)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	xmlSetProp (node, "pixtype", "stock");
	xmlNodeSetContent (node, name);
}

void
bonobo_ui_util_xml_set_fname (xmlNode     *node,
			      const char  *name)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);
	
	xmlSetProp (node, "pixtype", "filename");
	xmlNodeSetContent (node, name);
}
