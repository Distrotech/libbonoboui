/* $Id */
/*
  Sample-Container Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  Copyright (C) 2000 Helix Code, Inc.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  (included in the RadioActive distribution in doc/GPL) as published by
  the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef SAMPLE_COMPONENT_IO_H
#define SAMPLE_COMPONENT_IO_H

#include <bonobo.h>

#include "container.h"
#include "component.h"

void component_load    (Component *component, Bonobo_Stream stream,
			CORBA_Environment *ev);

void component_save    (Component *component, Bonobo_Stream stream,
			CORBA_Environment *ev);

void component_save_id (Component *component, Bonobo_Stream stream,
			CORBA_Environment *ev);

#endif
