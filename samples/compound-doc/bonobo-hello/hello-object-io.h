/* $Id$ */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
  
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

#ifndef HELLO_OBJECT_IO_H
#define HELLO_OBJECT_IO_H

#include "hello-object.h"

void hello_object_pstream_load (BonoboPersistStream * ps,
				const Bonobo_Stream stream,
				Bonobo_Persist_ContentType type,
				void *data, CORBA_Environment *ev);
void hello_object_pstream_save (BonoboPersistStream * ps,
				const Bonobo_Stream stream,
				Bonobo_Persist_ContentType type,
				void *data, CORBA_Environment *ev);
CORBA_long hello_object_pstream_get_max_size (BonoboPersistStream *ps,
					      void *data,
					      CORBA_Environment *ev);
Bonobo_Persist_ContentTypeList *hello_object_pstream_get_types (BonoboPersistStream *ps,
								void *closure,
								CORBA_Environment *ev);

#endif
