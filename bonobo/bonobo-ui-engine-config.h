/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-ui-engine-config.h: The Bonobo UI/XML Sync engine user config code
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2001 Helix Code, Inc.
 */

#ifndef _BONOBO_UI_ENGINE_CONFIG_H_
#define _BONOBO_UI_ENGINE_CONFIG_H_

BEGIN_GNOME_DECLS

#include <bonobo/bonobo-ui-engine.h>

void bonobo_ui_engine_config_connect     (GtkWidget      *widget,
					  BonoboUIEngine *engine,
					  const char     *path,
					  const char     *popup_xml);

void bonobo_ui_engine_config_add_node    (BonoboUIEngine *engine,
					  BonoboUINode   *node);

void bonobo_ui_engine_config_remove_node (BonoboUIEngine *engine,
					  BonoboUINode   *node);
				       
END_GNOME_DECLS

#endif /* _BONOBO_UI_ENGINE_H_ */
