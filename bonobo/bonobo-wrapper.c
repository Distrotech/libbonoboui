/* Wrapper for plug/socket children in Bonobo
 *
 * Copyright (C) 1999 the Free Software Foundation
 *
 * Author:
 *    Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <bonobo/gnome-wrapper.h>


static void gnome_wrapper_class_init (GnomeWrapperClass *class);
static void gnome_wrapper_init (GnomeWrapper *wrapper);

static void gnome_wrapper_map (GtkWidget *widget);
static void gnome_wrapper_unmap (GtkWidget *widget);
static void gnome_wrapper_realize (GtkWidget *widget);
static void gnome_wrapper_unrealize (GtkWidget *widget);
static void gnome_wrapper_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gnome_wrapper_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

static GtkBinClass *parent_class;


/**
 * gnome_wrapper_get_type:
 * @void: 
 * 
 * Generates a unique type ID for GnomeWrapperClass.
 * 
 * Return value: the type ID for GnomeWrapperClass.
 **/
GtkType
gnome_wrapper_get_type (void)
{
	static GtkType wrapper_type = 0;

	if (!wrapper_type) {
		static const GtkTypeInfo wrapper_info = {
			"GnomeWrapper",
			sizeof (GnomeWrapper),
			sizeof (GnomeWrapperClass),
			(GtkClassInitFunc) gnome_wrapper_class_init,
			(GtkObjectInitFunc) gnome_wrapper_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		wrapper_type = gtk_type_unique (gtk_bin_get_type (), &wrapper_info);
	}

	return wrapper_type;
}

/* Standard class initialization function */
static void
gnome_wrapper_class_init (GnomeWrapperClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (GTK_TYPE_BIN);

	widget_class->map = gnome_wrapper_map;
	widget_class->unmap = gnome_wrapper_unmap;
	widget_class->realize = gnome_wrapper_realize;
	widget_class->unrealize = gnome_wrapper_unrealize;
	widget_class->size_request = gnome_wrapper_size_request;
	widget_class->size_allocate = gnome_wrapper_size_allocate;
}

/* Standard object initialization function */
static void
gnome_wrapper_init (GnomeWrapper *wrapper)
{
	wrapper->covered = TRUE;
}

/* Map handler for the wrapper widget.  We map the child, then the normal
 * widget->window, and then the wrapper->cover only if it is active.
 */
static void
gnome_wrapper_map (GtkWidget *widget)
{
	GnomeWrapper *wrapper;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));

	wrapper = GNOME_WRAPPER (widget);
	GTK_WIDGET_SET_FLAGS (wrapper, GTK_MAPPED);

	if (wrapper->bin.child
	    && GTK_WIDGET_VISIBLE (wrapper->bin.child)
	    && !GTK_WIDGET_MAPPED (wrapper->bin.child))
		gtk_widget_map (wrapper->bin.child);

	gdk_window_show (widget->window);

	if (wrapper->covered)
		gdk_window_show (wrapper->cover);
}

/* Unmap handler for the wrapper widget */
static void
gnome_wrapper_unmap (GtkWidget *widget)
{
	GnomeWrapper *wrapper;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));

	wrapper = GNOME_WRAPPER (widget);
	GTK_WIDGET_UNSET_FLAGS (wrapper, GTK_MAPPED);

	gdk_window_hide (widget->window);

	if (wrapper->covered)
		gdk_window_hide (wrapper->cover);

	if (wrapper->bin.child && GTK_WIDGET_MAPPED (wrapper->bin.child))
		gtk_widget_unmap (wrapper->bin.child);
}

/* Realize handler for the wrapper widget.  We create the widget->window, which
 * is the child's container, and the wrapper->cover, which is the cover window.
 * This is a bit special in that both windows are direct children of the parent
 * widget's window.
 */
static void
gnome_wrapper_realize (GtkWidget *widget)
{
	GnomeWrapper *wrapper;
	GdkWindow *parent_window;
	GdkWindowAttr attributes;
	int attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));

	wrapper = GNOME_WRAPPER (widget);
	GTK_WIDGET_SET_FLAGS (wrapper, GTK_REALIZED);

	parent_window = gtk_widget_get_parent_window (widget);

	/* Child's window */

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (parent_window, &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, wrapper);

	/* Cover window */

	attributes.wclass = GDK_INPUT_ONLY;

	wrapper->cover = gdk_window_new (parent_window, &attributes, attributes_mask);
	gdk_window_set_events (wrapper->cover, GDK_BUTTON_PRESS_MASK);
	gdk_window_set_user_data (wrapper->cover, wrapper);

	/* Style */

	widget->style = gtk_style_attach (widget->style, widget->window);
	
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

/* Unrealize handler for the wrapper widget.  We destroy the cover window and let
 * the default handler do the rest.
 */
static void
gnome_wrapper_unrealize (GtkWidget *widget)
{
	GnomeWrapper *wrapper;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));

	wrapper = GNOME_WRAPPER (widget);

	gdk_window_set_user_data (wrapper->cover, NULL);
	gdk_window_destroy (wrapper->cover);
	wrapper->cover = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

/* Size request handler for the wrapper widget.  We simply use the child's
 * requisition.
 */
static void
gnome_wrapper_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomeWrapper *wrapper;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));
	g_return_if_fail (requisition != NULL);

	wrapper = GNOME_WRAPPER (widget);

	if (wrapper->bin.child)
		gtk_widget_size_request (wrapper->bin.child, requisition);
	else {
		requisition->width = 1;
		requisition->height = 1;
	}
}

/* Size allocate handler for the wrapper widget.  We simply use the allocation
 * and don't pay attention to the border_width.
 */
static void
gnome_wrapper_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GnomeWrapper *wrapper;
	GtkAllocation child_allocation;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (widget));
	g_return_if_fail (allocation != NULL);

	wrapper = GNOME_WRAPPER (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (wrapper)) {
		gdk_window_move_resize (widget->window,
					widget->allocation.x,
					widget->allocation.y,
					widget->allocation.width,
					widget->allocation.height);
		gdk_window_move_resize (wrapper->cover,
					widget->allocation.x,
					widget->allocation.y,
					widget->allocation.width,
					widget->allocation.height);
	}

	if (wrapper->bin.child && GTK_WIDGET_VISIBLE (wrapper->bin.child)) {
		child_allocation.x = 0;
		child_allocation.y = 0;
		child_allocation.width = widget->allocation.width;
		child_allocation.height = widget->allocation.height;

		gtk_widget_size_allocate (wrapper->bin.child, &child_allocation);
	}
}

/**
 * gnome_wrapper_new:
 * @void: 
 * 
 * Creates a new wrapper widget.  It starts covered by default.
 * 
 * Return value: The newly-created wrapper widget.
 **/
GtkWidget *
gnome_wrapper_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_wrapper_get_type ()));
}

/**
 * gnome_wrapper_set_covered:
 * @wrapper: A wrapper widget
 * @covered: Whether it should be covered or not
 * 
 * Sets the covered status of a wrapper widget by showing or hiding the cover
 * window as appropriate.
 **/
void
gnome_wrapper_set_covered (GnomeWrapper *wrapper, gboolean covered)
{
	g_return_if_fail (wrapper != NULL);
	g_return_if_fail (GNOME_IS_WRAPPER (wrapper));

	if (wrapper->covered && !covered) {
		wrapper->covered = FALSE;

		if (GTK_WIDGET_MAPPED (wrapper))
			gdk_window_hide (wrapper->cover);
	} else if (!wrapper->covered && covered) {
		wrapper->covered = TRUE;

		if (GTK_WIDGET_MAPPED (wrapper))
			gdk_window_show (wrapper->cover);
	}
}

/**
 * gnome_wrapper_is_covered:
 * @wrapper: A wrapper widget.
 * 
 * Queries the covered status of a wrapper widget.
 * 
 * Return value: Whether the wrapper widget is covering its child or not.
 **/
gboolean
gnome_wrapper_is_covered (GnomeWrapper *wrapper)
{
	g_return_val_if_fail (wrapper != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_WRAPPER (wrapper), FALSE);

	return wrapper->covered;
}
