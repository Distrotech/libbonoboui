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

G_BEGIN_DECLS

#include <bonobo/bonobo-ui-xml.h>
#include <bonobo/bonobo-ui-engine.h>

#define BONOBO_TYPE_UI_ENGINE_CONFIG            (bonobo_ui_engine_config_get_type ())
#define BONOBO_UI_ENGINE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BONOBO_TYPE_UI_ENGINE_CONFIG, BonoboUIEngineConfig))
#define BONOBO_UI_ENGINE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BONOBO_TYPE_UI_ENGINE_CONFIG, BonoboUIEngineConfigClass))
#define BONOBO_IS_UI_ENGINE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BONOBO_TYPE_UI_ENGINE_CONFIG))
#define BONOBO_IS_UI_ENGINE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BONOBO_TYPE_UI_ENGINE_CONFIG))

typedef struct _BonoboUIEngineConfigPrivate BonoboUIEngineConfigPrivate;

typedef struct {
	GObject parent;

	BonoboUIEngineConfigPrivate *priv;
} BonoboUIEngineConfig;

typedef struct {
	GObjectClass parent_class;

	gpointer       dummy;
} BonoboUIEngineConfigClass;

GType bonobo_ui_engine_config_get_type  (void) G_GNUC_CONST;

BonoboUIEngineConfig *
        bonobo_ui_engine_config_construct (BonoboUIEngineConfig   *config,
					   BonoboUIEngine         *engine,
					   GtkWindow              *opt_parent);

BonoboUIEngineConfig *
        bonobo_ui_engine_config_new       (BonoboUIEngine         *engine,
					   GtkWindow              *opt_parent);

typedef char*(*BonoboUIEngineConfigFn)    (BonoboUIEngineConfig   *config,
					   BonoboUINode           *config_node,
					   BonoboUIEngine         *popup_engine);

typedef void (*BonoboUIEngineConfigVerbFn)(BonoboUIEngineConfig   *config,
					   const char             *path,
					   const char             *opt_state,
					   BonoboUIEngine         *popup_engine,
					   BonoboUINode           *popup_node);

void    bonobo_ui_engine_config_connect   (GtkWidget                  *widget,
					   BonoboUIEngine             *engine,
					   const char                 *path,
					   BonoboUIEngineConfigFn      config_fn,
					   BonoboUIEngineConfigVerbFn  verb_fn);
void    bonobo_ui_engine_config_serialize (BonoboUIEngineConfig   *config);
void    bonobo_ui_engine_config_hydrate   (BonoboUIEngineConfig   *config);
void    bonobo_ui_engine_config_add       (BonoboUIEngineConfig   *config,
					   const char             *path,
					   const char             *attr,
					   const char             *value);
void    bonobo_ui_engine_config_remove    (BonoboUIEngineConfig   *config,
					   const char             *path,
					   const char             *attr);

void    bonobo_ui_engine_config_configure (BonoboUIEngineConfig   *config);

BonoboUIEngine *bonobo_ui_engine_config_get_engine (BonoboUIEngineConfig *config);

G_END_DECLS

#endif /* _BONOBO_UI_ENGINE_CONFIG_H_ */



