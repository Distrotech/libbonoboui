/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo UI internal prototypes / helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _BONOBO_UI_PRIVATE_H_
#define _BONOBO_UI_PRIVATE_H_

#undef DEBUG_UI

void bonobo_ui_preferences_shutdown (void);


#ifndef   DEBUG_UI

static inline void dprintf (const char *format, ...) { };

#else  /* DEBUG_UI */

#include <stdio.h>

#define dprintf(format...) fprintf(stderr, format)

#endif /* DEBUG_CONTROL */

#endif /* _BONOBO_UI_PRIVATE_H_ */

