/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Main include file for the Bonobo component model
 *
 * FIXME: We should try to optimize the order of the include
 * files here to minimize repeated inclussion of files.
 *
 * Copyright 1999 Helix Code, Inc.
 */

#ifndef BONOBO_H
#define BONOBO_H 1

#include <bonobo/bonobo-object.h>

#include <bonobo/bonobo-moniker.h>
#include <bonobo/bonobo-moniker-client.h>

#include <bonobo/bonobo-container.h>
#include <bonobo/bonobo-object-client.h>
#include <bonobo/bonobo-client-site.h>

#include <bonobo/bonobo-property-bag.h>
#include <bonobo/bonobo-property-bag-client.h>

#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-control-frame.h>

#include <bonobo/bonobo-view.h>
#include <bonobo/bonobo-generic-factory.h>

#include <bonobo/bonobo-embeddable.h>
#include <bonobo/bonobo-embeddable-factory.h>

#include <bonobo/bonobo-main.h>

#include <bonobo/bonobo-stream.h>
#include <bonobo/bonobo-stream-client.h>
#include <bonobo/bonobo-stream-fs.h>

#include <bonobo/bonobo-persist.h>
#include <bonobo/bonobo-persist-file.h>
#include <bonobo/bonobo-persist-stream.h>

#include <bonobo/bonobo-ui-handler.h>

#include <bonobo/bonobo-object-io.h>

#include <bonobo/bonobo-progressive.h>

#include <bonobo/bonobo-storage.h>

#include <bonobo/bonobo-selector.h>

#include <bonobo/bonobo-widget.h>

#endif
