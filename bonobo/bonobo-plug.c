/* Bonobo-Plug.c: A private version of GtkPlug
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* By Owen Taylor <otaylor@gtk.org>              98/4/4 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include <stdio.h>
#include "gdk/gdkx.h"
#include "gdk/gdkkeysyms.h"
#include "bonobo/bonobo-plug.h"

/* Private part of the BonoboPlug structure */
struct _BonoboPlugPrivate {
	GdkWindow *socket_window;
	gint same_app;

	/* Whether we have the focus.  We cannot use the GTK_HAS_FOCUS flag
	 * because we are not a GTK_CAN_FOCUS widget and it would make
	 * GtkContainer::focus() skip us immediately when trying to change the
	 * focus within the plug's children.
	 */
	guint has_focus : 1;
};

/* From Tk */
#define EMBEDDED_APP_WANTS_FOCUS NotifyNormal+20

static GtkWindowClass *parent_class = NULL;

/**
 * bonobo_plug_construct:
 * @plug: The #BonoboPlug.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the creation of a #BonoboPlug widget. This function
 * will generally only be used by classes deriving
 * from #BonoboPlug.
 */
void
bonobo_plug_construct (BonoboPlug *plug, guint32 socket_id)
{
	BonoboPlugPrivate *priv;

	g_return_if_fail (plug != NULL);
	g_return_if_fail (BONOBO_IS_PLUG (plug));

	priv = plug->priv;

	priv->socket_window = gdk_window_lookup (socket_id);
	priv->same_app = TRUE;

	if (priv->socket_window == NULL) {
		priv->socket_window = gdk_window_foreign_new (socket_id);
		priv->same_app = FALSE;
	}
}

/**
 * bonobo_plug_new:
 * @socket_id: the XID of the socket's window.
 *
 * Create a new plug widget inside the #GtkSocket identified
 * by @socket_id.
 *
 * Returns: the new #BonoboPlug widget.
 */
GtkWidget*
bonobo_plug_new (guint32 socket_id)
{
	BonoboPlug *plug;

	plug = BONOBO_PLUG (gtk_type_new (bonobo_plug_get_type ()));
	bonobo_plug_construct (plug, socket_id);
	return GTK_WIDGET (plug);
}

/* Destroy handler for the plug widget */
static void
bonobo_plug_destroy (GtkObject *object)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BONOBO_IS_PLUG (object));

	plug = BONOBO_PLUG (object);
	priv = plug->priv;

	g_free (priv);
	plug->priv = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
bonobo_plug_unrealize (GtkWidget *widget)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_PLUG (widget));

	plug = BONOBO_PLUG (widget);
	priv = plug->priv;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);

	if (priv->socket_window != NULL) {
		gdk_window_set_user_data (priv->socket_window, NULL);
		gdk_window_unref (priv->socket_window);
		priv->socket_window = NULL;
	}
}

static void
bonobo_plug_realize (GtkWidget *widget)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;
	GtkWindow *window;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (BONOBO_IS_PLUG (widget));

	plug = BONOBO_PLUG (widget);
	priv = plug->priv;

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	window = GTK_WINDOW (widget);

	attributes.window_type = GDK_WINDOW_CHILD;	/* XXX GDK_WINDOW_PLUG ? */
	attributes.title = window->title;
	attributes.wmclass_name = window->wmclass_name;
	attributes.wmclass_class = window->wmclass_class;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;

	/* this isn't right - we should match our parent's visual/colormap.
	 * though that will require handling "foreign" colormaps */
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_EXPOSURE_MASK |
				  GDK_KEY_PRESS_MASK |
				  GDK_ENTER_NOTIFY_MASK |
				  GDK_LEAVE_NOTIFY_MASK |
				  GDK_FOCUS_CHANGE_MASK |
				  GDK_STRUCTURE_MASK);

	attributes_mask = GDK_WA_VISUAL | GDK_WA_COLORMAP;
	attributes_mask |= (window->title ? GDK_WA_TITLE : 0);
	attributes_mask |= (window->wmclass_name ? GDK_WA_WMCLASS : 0);

	gdk_error_trap_push ();
	widget->window = gdk_window_new (priv->socket_window,
					 &attributes, attributes_mask);
	gdk_flush ();
	if (gdk_error_trap_pop ()) /* Uh-oh */
	{
		gdk_error_trap_push ();
		gdk_window_destroy (widget->window);
		gdk_flush ();
		gdk_error_trap_pop ();
		widget->window = gdk_window_new (NULL, &attributes, attributes_mask);
	}

	((GdkWindowPrivate *)widget->window)->window_type = GDK_WINDOW_TOPLEVEL;
	gdk_window_set_user_data (widget->window, window);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
bonobo_plug_forward_key_press (BonoboPlug *plug, GdkEventKey *event)
{
	BonoboPlugPrivate *priv;
	XEvent xevent;

	priv = plug->priv;

	xevent.xkey.type = KeyPress;
	xevent.xkey.display = GDK_WINDOW_XDISPLAY (GTK_WIDGET(plug)->window);
	xevent.xkey.window = GDK_WINDOW_XWINDOW (priv->socket_window);
	xevent.xkey.root = GDK_ROOT_WINDOW (); /* FIXME */
	xevent.xkey.time = event->time;
	/* FIXME, the following might cause big problems for
	 * non-GTK apps */
	xevent.xkey.x = 0;
	xevent.xkey.y = 0;
	xevent.xkey.x_root = 0;
	xevent.xkey.y_root = 0;
	xevent.xkey.state = event->state;
	xevent.xkey.keycode =  XKeysymToKeycode(GDK_DISPLAY(),
						event->keyval);
	xevent.xkey.same_screen = TRUE; /* FIXME ? */

	gdk_error_trap_push ();
	XSendEvent (gdk_display,
		    GDK_WINDOW_XWINDOW (priv->socket_window),
		    False, NoEventMask, &xevent);
	gdk_flush ();
	gdk_error_trap_pop ();
}

/* Key_press_event handle for the plug widget */
static gint
bonobo_plug_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;
	GtkWindow *window;
	GtkDirectionType direction = 0;
	gint return_val;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_PLUG (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	plug = BONOBO_PLUG (widget);
	priv = plug->priv;

	window = GTK_WINDOW (widget);

	if (!priv->has_focus) {
		bonobo_plug_forward_key_press (plug, event);
		return FALSE;
	}

	return_val = FALSE;
	if (window->focus_widget
	    && window->focus_widget != widget
	    && GTK_WIDGET_IS_SENSITIVE (window->focus_widget))
		return_val = gtk_widget_event (window->focus_widget, (GdkEvent*) event);

	if (!return_val)
		switch (event->keyval) {
		case GDK_space:
			if (window->focus_widget) {
				if (GTK_WIDGET_IS_SENSITIVE (window->focus_widget))
					return_val = gtk_widget_activate (window->focus_widget);
			}
			break;

		case GDK_Return:
		case GDK_KP_Enter:
			if (window->default_widget && GTK_WIDGET_IS_SENSITIVE (window->default_widget)
			    && (!window->focus_widget
				|| !GTK_WIDGET_RECEIVES_DEFAULT (window->focus_widget)))
				return_val = gtk_widget_activate (window->default_widget);
			else if (window->focus_widget) {
				if (GTK_WIDGET_IS_SENSITIVE (window->focus_widget))
					return_val = gtk_widget_activate (window->focus_widget);
			}
			break;

		case GDK_Up:
		case GDK_Down:
		case GDK_Left:
		case GDK_Right:
		case GDK_Tab:
			switch (event->keyval) {
			case GDK_Up:
				direction = GTK_DIR_UP;
				break;
			case GDK_Down:
				direction = GTK_DIR_DOWN;
				break;
			case GDK_Left:
				direction = GTK_DIR_LEFT;
				break;
			case GDK_Right:
				direction = GTK_DIR_RIGHT;
				break;
			case GDK_Tab:
				if (event->state & GDK_SHIFT_MASK)
					direction = GTK_DIR_TAB_BACKWARD;
				else
					direction = GTK_DIR_TAB_FORWARD;
				break;
			default :
				direction = GTK_DIR_UP; /* never reached, but makes compiler happy */
			}

			if (gtk_container_focus (GTK_CONTAINER (widget), direction))
				g_assert (GTK_CONTAINER (widget)->focus_child != NULL);
			else {
				gtk_window_set_focus (window, NULL);
				bonobo_plug_forward_key_press (plug, event);
			}

			return_val = TRUE;
			break;
		}

	/* If we havn't handled it pass it on to our socket, since it might be a
	 * keybinding or something interesting.
	 */
	if (!return_val)
		bonobo_plug_forward_key_press (plug, event);

	return return_val;
}

/* Focus_in_event handler for the plug widget */
static gint
bonobo_plug_focus_in_event (GtkWidget *widget, GdkEventFocus *event)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;
	GtkWindow *window;
	GdkEventFocus fevent;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_PLUG (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	plug = BONOBO_PLUG (widget);
	priv = plug->priv;
#if 0
	fprintf (stderr, "bonobo_plug_focus_in_event: focus in event!\n");
#endif
	/* It appears spurious focus in events can occur when
	 *  the window is hidden. So we'll just check to see if
	 *  the window is visible before actually handling the
	 *  event
	 */
	if (GTK_WIDGET_VISIBLE (widget)) {
		priv->has_focus = TRUE;

		window = GTK_WINDOW (widget);
		if (window->focus_widget && !GTK_WIDGET_HAS_FOCUS (window->focus_widget)) {
			fevent.type = GDK_FOCUS_CHANGE;
			fevent.window = window->focus_widget->window;
			fevent.in = TRUE;

			gtk_widget_event (window->focus_widget, (GdkEvent*) &fevent);
		}
	}

	return FALSE;
}

/* Focus_out_event handler for the plug widget */
static gint
bonobo_plug_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;
	GtkWindow *window;
	GdkEventFocus fevent;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (BONOBO_IS_PLUG (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	plug = BONOBO_PLUG (widget);
	priv = plug->priv;

	priv->has_focus = FALSE;

	window = GTK_WINDOW (widget);
#if 0
	fprintf (stderr, "bonobo_plug_focus_out_event: focus out event!\n");
#endif
	if (window->focus_widget && GTK_WIDGET_HAS_FOCUS (window->focus_widget))
	{
		fevent.type = GDK_FOCUS_CHANGE;
		fevent.window = window->focus_widget->window;
		fevent.in = FALSE;

		gtk_widget_event (window->focus_widget, (GdkEvent*) &fevent);
	}

	return FALSE;
}

/* Set_focus handler for the plug widget */
static void
bonobo_plug_set_focus (GtkWindow *window, GtkWidget *focus)
{
	BonoboPlug *plug;
	BonoboPlugPrivate *priv;
	GdkEventFocus event;

	g_return_if_fail (window != NULL);
	g_return_if_fail (BONOBO_IS_PLUG (window));

	plug = BONOBO_PLUG (window);
	priv = plug->priv;

	if (focus && !GTK_WIDGET_CAN_FOCUS (focus))
		return;

	if (window->focus_widget != focus) {
		if (window->focus_widget) {
			event.type = GDK_FOCUS_CHANGE;
			event.window = window->focus_widget->window;
			event.in = FALSE;

			gtk_widget_event (window->focus_widget, (GdkEvent*) &event);
		}

		window->focus_widget = focus;

		if (window->focus_widget) {
			event.type = GDK_FOCUS_CHANGE;
			event.window = window->focus_widget->window;
			event.in = TRUE;

			gtk_widget_event (window->focus_widget, (GdkEvent*) &event);
		}
	}

	/* Ask for focus from parent */

	if (focus && !priv->has_focus) {
		XEvent xevent;

		xevent.xfocus.type = FocusIn;
		xevent.xfocus.display = GDK_WINDOW_XDISPLAY (GTK_WIDGET(plug)->window);
		xevent.xfocus.window = GDK_WINDOW_XWINDOW (priv->socket_window);
		xevent.xfocus.mode = EMBEDDED_APP_WANTS_FOCUS;
		xevent.xfocus.detail = FALSE; /* Don't force */

		gdk_error_trap_push ();
		XSendEvent (gdk_display,
			    GDK_WINDOW_XWINDOW (priv->socket_window),
			    False, NoEventMask, &xevent);
		gdk_flush ();
		gdk_error_trap_pop ();
	}
}

static void
bonobo_plug_init (BonoboPlug *plug)
{
	BonoboPlugPrivate *priv;
	GtkWindow *window;

	priv = g_new0 (BonoboPlugPrivate, 1);
	plug->priv = priv;

	window = GTK_WINDOW (plug);

	window->type = GTK_WINDOW_TOPLEVEL;
	window->auto_shrink = TRUE;

	priv->has_focus = FALSE;
}

static void
bonobo_plug_class_init (BonoboPlugClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkWindowClass *window_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	window_class = (GtkWindowClass *) class;

	parent_class = gtk_type_class (gtk_window_get_type ());

	object_class->destroy = bonobo_plug_destroy;

	widget_class->realize = bonobo_plug_realize;
	widget_class->unrealize = bonobo_plug_unrealize;
	widget_class->key_press_event = bonobo_plug_key_press_event;
	widget_class->focus_in_event = bonobo_plug_focus_in_event;
	widget_class->focus_out_event = bonobo_plug_focus_out_event;

	window_class->set_focus = bonobo_plug_set_focus;
}

guint
bonobo_plug_get_type ()
{
	static guint plug_type = 0;

	if (!plug_type)
	{
		static const GtkTypeInfo plug_info =
		{
			"BonoboPlug",
			sizeof (BonoboPlug),
			sizeof (BonoboPlugClass),
			(GtkClassInitFunc) bonobo_plug_class_init,
			(GtkObjectInitFunc) bonobo_plug_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		plug_type = gtk_type_unique (gtk_window_get_type (), &plug_info);
	}

	return plug_type;
}
