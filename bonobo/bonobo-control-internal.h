/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Bonobo control internal prototypes / helpers
 *
 * Author:
 *   Michael Meeks (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef _BONOBO_CONTROL_INTERNAL_H_
#define _BONOBO_CONTROL_INTERNAL_H_

#define DEBUG_CONTROL

#ifndef   DEBUG_CONTROL

static inline void dprintf (const char *format, ...) { };

#else  /* DEBUG_CONTROL */

#include <stdio.h>

#define dprintf(format...) fprintf(stderr, format)

#endif /* DEBUG_CONTROL */

#endif /* _BONOBO_CONTROL_INTERNAL_H_ */
