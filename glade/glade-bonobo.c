/* -*- Mode: C; c-basic-offset: 4 -*-
 * libglade - a library for building interfaces from XML files at runtime
 * Copyright (C) 1998-2001  James Henstridge <james@daa.com.au>
 *
 * glade-bonobo.c: support for bonobo widgets in libglade.
 * Copyright (C) 2000 Helix Code, Inc.
 * Copyright (C) 2001 James Henstridge
 *
 * Author:
 *      Michael Meeks (michael@helixcode.com)
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
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

/* this file is only built if GNOME support is enabled */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glade/glade-init.h>
#include <glade/glade-build.h>

#include <libbonoboui.h>

#ifndef ENABLE_NLS
/* a slight optimisation when gettext is off */
#  define glade_xml_gettext(xml, msgid) (msgid)
#endif
#undef _
#define _(msgid) (glade_xml_gettext(xml, msgid))

static GtkWidget *
glade_bonobo_widget_new (GladeXML *xml, GType widget_type,
			 GladeWidgetInfo *info)
{
    const gchar *control_moniker = NULL;
    GtkWidget *widget;
    BonoboControlFrame *cf;
    Bonobo_PropertyBag pb;
    GList *tmp;
    gint i;

    for (i = 0; i < info->n_properties; i++)
	if (!strcmp (info->properties[i].name, "moniker")) {
	    control_moniker = info->properties[i].value;
	    break;
	}

    if (!control_moniker) {
	g_warning (G_STRLOC " BonoboWidget doesn't have moniker property");
	return NULL;
    }
    widget = bonobo_widget_new_control (control_moniker, CORBA_OBJECT_NIL);

    if (!widget) {
	g_warning (G_STRLOC " unknown bonobo control '%s'", control_moniker);
	return NULL;
    }

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
	CORBA_TypeCode tc;

	if (!strcmp (name, "moniker"))
	    continue;

	tc  = bonobo_property_bag_client_get_property_type (pb, name, NULL);

	switch (tc->kind) {
	case CORBA_tk_boolean:
	    bonobo_property_bag_client_set_value_gboolean (pb, name,
				value[0] == 'T' || value[0] == 'y', NULL);
	    break;
	case CORBA_tk_string:
	    bonobo_property_bag_client_set_value_string (pb, name, value,NULL);
	    break;
	case CORBA_tk_long:
	    bonobo_property_bag_client_set_value_glong (pb, name,
							strtol (value, NULL,0),
							NULL);
	    break;
	case CORBA_tk_float:
	    bonobo_property_bag_client_set_value_gfloat (pb, name,
							 strtod (value, NULL),
							 NULL);
	    break;
	case CORBA_tk_double:
	    bonobo_property_bag_client_set_value_gdouble (pb, name,
							  strtod (value, NULL),
							  NULL);
	    break;
	default:
	    g_warning ("Unhandled type %d", tc->kind);
	    break;
	}
    }

    gtk_widget_show (widget);
    return widget;
}

static GladeWidgetBuildData widget_data[] = {
    { "BonoboWidget", glade_bonobo_widget_new, NULL,
      bonobo_widget_get_type },
    { NULL, NULL, NULL, 0, 0 }
};

/* this macro puts a version check function into the module */
GLADE_MODULE_CHECK_INIT

void
glade_module_register_widgets (void)
{
    glade_provide ("bonobo");
    glade_register_widgets (widget_data);
}
