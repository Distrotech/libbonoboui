/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-main.h: Bonobo Main
 *
 * Author:
 *    Miguel de Icaza  (miguel@gnu.org)
 *    Nat Friedman     (nat@nat.org)
 *    Peter Wainwright (prw@wainpr.demo.co.uk)
 *
 * Copyright 1999 Helix Code, Inc.
 */

#ifndef __BONOBO_UI_MAIN_H__
#define __BONOBO_UI_MAIN_H__ 1

#include <bonobo/bonobo-main.h>

G_BEGIN_DECLS

gboolean   bonobo_ui_is_initialized     (void);
gboolean   bonobo_ui_init               (const gchar *app_name,
					 const gchar *app_version,
					 int *argc, char **argv);
void       bonobo_ui_main               (void);
gboolean   bonobo_ui_init_full          (const gchar *app_name,
					 const gchar *app_version,
					 int *argc, char **argv,
                                         CORBA_ORB orb,
                                         PortableServer_POA poa,
                                         PortableServer_POAManager manager,
					 gboolean full_init);
void       bonobo_setup_x_error_handler (void);
/* internal */
int        bonobo_ui_debug_shutdown     (void);

G_END_DECLS

#endif /* __BONOBO_UI_MAIN_H__ */
