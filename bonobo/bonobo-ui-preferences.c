/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-ui-preferences.c: private wrappers for global UI preferences.
 *
 * Authors:
 *     Michael Meeks (michael@helixcode.com)
 *     Martin Baulig (martin@home-of-linux.org)
 *
 * Copyright 2001 Ximian, Inc.
 */

#include <config.h>
#include <gconf.h>
#include "bonobo/bonobo-ui-preferences.h"

#define DEFINE_DESKTOP_PROP_BOOLEAN(c_name, prop_name)  \
gboolean                                                \
gnome_preferences_get_ ## c_name (void)                 \
{                                                       \
	return gconf_get_value_gboolean ("/desktop/gnome/interface/" prop_name); \
}

DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_labels,     "toolbar-labels");
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_detachable, "toolbar-detachable");
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_relief,     "toolbar-relief");

DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_icons,   "menus-have-icons");
DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_tearoff, "menus-have-tearoff");
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_detachable, "menubar-detachable");
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_relief,     "menubar-relief");
