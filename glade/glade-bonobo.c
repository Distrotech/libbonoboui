/*
 * glade-bonobo.c: support for bonobo widgets in libglade.
 *
 * Author:
 *      Michael Meeks (michael@helixcode.com)
 *
 * Copyright (C) 2000,2001 Ximian, Inc., 2001 James Henstridge.
 */
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <libbonoboui.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>

static GtkWidget *
glade_bonobo_widget_new (GladeXML        *xml,
			 GType            widget_type,
			 GladeWidgetInfo *info)
{
	const gchar *control_moniker = NULL;
	GtkWidget *widget;
	GObjectClass *oclass;
	BonoboControlFrame *cf;
	Bonobo_PropertyBag pb;
	gint i;

	for (i = 0; i < info->n_properties; i++) {
		if (!strcmp (info->properties[i].name, "moniker")) {
			control_moniker = info->properties[i].value;
			break;
		}
	}

	if (!control_moniker) {
		g_warning (G_STRLOC " BonoboWidget doesn't have moniker property");
		return NULL;
	}
	widget = bonobo_widget_new_control (
		control_moniker, CORBA_OBJECT_NIL);

	if (!widget) {
		g_warning (G_STRLOC " unknown bonobo control '%s'", control_moniker);
		return NULL;
	}

	oclass = G_OBJECT_GET_CLASS (widget);

	cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (widget));

	if (!cf) {
		g_warning ("control '%s' has no frame", control_moniker);
		gtk_widget_unref (widget);
		return NULL;
	}

	pb = bonobo_control_frame_get_control_property_bag (cf, NULL);
	if (pb == CORBA_OBJECT_NIL)
		return widget;

	for (i = 0; i < info->n_properties; i++) {
		const gchar *name = info->properties[i].name;
		const gchar *value = info->properties[i].value;
		GParamSpec *pspec;

		if (!strcmp (name, "moniker"))
			continue;

		pspec = g_object_class_find_property (oclass, name);
		if (pspec) {
			GValue gvalue = { 0 };

			if (glade_xml_set_value_from_string(xml, pspec, value, &gvalue)) {
				g_object_set_property(G_OBJECT(widget), name, &gvalue);
				g_value_unset(&gvalue);
			}
		} else if (pb != CORBA_OBJECT_NIL) {
			CORBA_TypeCode tc =
				bonobo_property_bag_client_get_property_type (pb, name, NULL);

			switch (tc->kind) {
			case CORBA_tk_boolean:
				bonobo_property_bag_client_set_value_gboolean (pb, name,
									       value[0] == 'T' || value[0] == 'y', NULL);
				break;
			case CORBA_tk_string:
				bonobo_property_bag_client_set_value_string (pb, name, value,
									     NULL);
				break;
			case CORBA_tk_long:
				bonobo_property_bag_client_set_value_glong (pb, name,
									    strtol (value, NULL,0), NULL);
				break;
			case CORBA_tk_float:
				bonobo_property_bag_client_set_value_gfloat (pb, name,
									     strtod (value, NULL), NULL);
				break;
			case CORBA_tk_double:
				bonobo_property_bag_client_set_value_gdouble (pb, name,
									      strtod (value, NULL),
									      NULL);
				break;
			default:
				g_warning ("Unhandled type %d for `%s'", tc->kind, name);
				break;
			}
		} else {
			g_warning ("could not handle property `%s'", name);
		}
	}

	return widget;
}

static GtkWidget *
bonobo_window_find_internal_child (GladeXML    *xml,
				   GtkWidget   *parent,
				   const gchar *childname)
{
	if (!strcmp (childname, "vbox")) {
		GtkWidget *ret;

		if ((ret = bonobo_window_get_contents (
			BONOBO_WINDOW (parent))))
			return ret;

		else {
			GtkWidget *box;

			box = gtk_vbox_new (FALSE, 0);
			
			bonobo_window_set_contents (
				BONOBO_WINDOW (parent), box);

			return box;
		}
	}

    return NULL;
}

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
	glade_provide ("bonobo");
	glade_register_widget (BONOBO_TYPE_WIDGET,
			       glade_bonobo_widget_new,
			       NULL, NULL);
	glade_register_widget (BONOBO_TYPE_WINDOW,
			       NULL, glade_standard_build_children,
			       bonobo_window_find_internal_child);
}
