/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-ui-preferences.c: private wrappers for global UI preferences.
 *
 * Authors:
 *     Michael Meeks (michael@ximian.com)
 *     Martin Baulig (martin@home-of-linux.org)
 *
 * Copyright 2001 Ximian, Inc.
 */

#include <config.h>
#include <gconf/gconf-client.h>

#include <bonobo/bonobo-ui-preferences.h>

/*
 *   Yes Gconf's C API sucks, yes bonobo-config is a far better
 * way to access configuration, yes I hate this code; Michael.
 */
static gboolean
get (const char *key, gboolean def)
{
	gboolean           ret;
	GError             *err = NULL;
	static GConfClient *client = NULL;

	if (!client)					
		client = gconf_client_get_default ();	

	ret = gconf_client_get_bool (client, key, &err); 

	if (err) {
		static int warned = 0;
		if (!warned++)
			g_warning ("Failed to get '%s': '%s'", key, err->message);
		g_error_free (err);
		ret = def;
	}

	return ret;
}

#define DEFINE_BONOBO_UI_PREFERENCE(c_name, prop_name, def)      \
gboolean                                                         \
bonobo_ui_preferences_get_ ## c_name (void)                      \
{                                                                \
	return get ("/desktop/gnome/interface/" prop_name, def); \
}

DEFINE_BONOBO_UI_PREFERENCE (toolbar_labels,     "toolbar-labels",     TRUE);
DEFINE_BONOBO_UI_PREFERENCE (toolbar_detachable, "toolbar-detachable", TRUE);
DEFINE_BONOBO_UI_PREFERENCE (toolbar_relief,     "toolbar-relief",     TRUE);

DEFINE_BONOBO_UI_PREFERENCE (menus_have_icons,   "menus-have-icons",   TRUE);
DEFINE_BONOBO_UI_PREFERENCE (menus_have_tearoff, "menus-have-tearoff", TRUE);
DEFINE_BONOBO_UI_PREFERENCE (menubar_detachable, "menubar-detachable", TRUE);
DEFINE_BONOBO_UI_PREFERENCE (menubar_relief,     "menubar-relief",     TRUE);
