/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo accessibility helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2002 Sun Microsystems, Inc.
 */

#include <atk/atkregistry.h>
#include <atk/atkobjectfactory.h>
#include <gtk/gtkwidget.h>
#include <bonobo-a11y.h>

static GType
bonobo_a11y_get_derived_type_for (GType widget_type,
				  GType parent_atk_type,
				  BonoboA11YClassInitFn class_init)
{
	GType type;
	GTypeInfo tinfo = { 0 };
	GTypeQuery query;
	char *type_name;
		
	/*
	 * Figure out the size of the class and instance 
	 * we are deriving from
	 */
	g_type_query (parent_atk_type, &query);

	tinfo.class_init    = (GClassInitFunc) class_init;
	tinfo.class_size    = query.class_size;
	tinfo.instance_size = query.instance_size;

	/* Make up a name */
	type_name = g_strconcat (g_type_name (widget_type),
				 "Accessible", NULL);

	/* Register the type */
	type = g_type_register_static (
		parent_atk_type, type_name, &tinfo, 0);

	g_free (type_name);
		
	return type;
}

AtkObject *
bonobo_a11y_create_accessible_for (GtkWidget            *widget,
				   const char           *gail_parent_class,
				   BonoboA11YClassInitFn class_init)
{
	AtkObject *accessible;
	GType type, parent_type, widget_type;
	static GQuark quark_accessible_object = 0;
	static GHashTable *type_hash = NULL;

	if (!quark_accessible_object)
		quark_accessible_object = g_quark_from_static_string (
			"gtk-accessible-object");

	accessible = g_object_get_qdata (
		G_OBJECT (widget), quark_accessible_object);

	if (accessible)
		return accessible;

	parent_type = g_type_from_name (gail_parent_class ?
					gail_parent_class : "GailWidget");
	g_return_val_if_fail (parent_type != G_TYPE_INVALID, NULL);

	if (!type_hash)
		type_hash = g_hash_table_new (NULL, NULL);

	widget_type = G_TYPE_FROM_INSTANCE (widget);
	type = (GType) g_hash_table_lookup (type_hash, (gpointer) widget_type);

	if (!type) {
		type = bonobo_a11y_get_derived_type_for (
			widget_type, parent_type, class_init);

		g_hash_table_insert (type_hash,
				     (gpointer) widget_type,
				     (gpointer) type);
	}

	g_return_val_if_fail (type != G_TYPE_INVALID, NULL);
					    
	accessible = g_object_new (type, NULL);

	atk_object_initialize (accessible, widget);

	g_object_set_qdata (G_OBJECT (widget), 
			    quark_accessible_object,
			    accessible);

	return accessible;
}
