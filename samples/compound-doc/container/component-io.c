/* $Id$ */
/*
  Bonobo-Hello Copyright (C) 2000 ÉRDI Gergõ <cactus@cactus.rulez.org>
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
  
  Authors:
     ÉRDI Gergõ <cactus@cactus.rulez.org>
     Michael Meeks <michael@helixcode.com>
*/

#include "component.h"
#include "component-io.h"

void
component_load (Component         *component,
		Bonobo_Stream      stream,
		CORBA_Environment *ev)
{
	Bonobo_PersistStream persist;

	if (stream == CORBA_OBJECT_NIL) {
		g_warning ("No stream to load component from");
		return;
	}

	/* Get the PersistStream interface of our component */
	persist = bonobo_object_client_query_interface (component->server,
							"IDL:Bonobo/PersistStream:1.0",
							NULL);
	if (ev->_major != CORBA_NO_EXCEPTION)
		return;

	if (persist == CORBA_OBJECT_NIL) {
		g_warning ("Component doesn't implement a PersistStream "
			   "interface, and it used to\n");
		return;
	}

	Bonobo_PersistStream_load (persist, stream, "", ev);

	/* See if we had any problems */
	if (ev->_major != CORBA_NO_EXCEPTION)
		gnome_warning_dialog (_
				      ("An exception occured while trying "
				       "to load data into the component with "
				       "PersistStorage"));

	Bonobo_Unknown_unref (persist, ev);
	CORBA_Object_release (persist, ev);
}

void
component_save (Component         *component,
		Bonobo_Stream      stream,
		CORBA_Environment *ev)
{
	Bonobo_PersistStream persist;

	/* Get the PersistStream interface of our component */
	persist = bonobo_object_client_query_interface (component->server,
							"IDL:Bonobo/PersistStream:1.0",
							ev);
	if (ev->_major != CORBA_NO_EXCEPTION ||
	    persist == CORBA_OBJECT_NIL) {
		printf ("This component doesn't implement PersistStream\n"
			"If you are planning on using PersistFile, don't.");
		return;
	}

	Bonobo_PersistStream_save (persist, stream, "", ev);

	/* See if we had any problems */
	if (ev->_major != CORBA_NO_EXCEPTION)
		gnome_warning_dialog (_
				      ("An exception occured while trying "
				       "to save data from the component with "
				       "PersistStorage"));

	else {
		Bonobo_Unknown_unref (persist, ev);
		CORBA_Object_release (persist, ev);
	}
}

void
component_save_id (Component         *component,
		   Bonobo_Stream      stream,
		   CORBA_Environment *ev)
{
	bonobo_stream_client_write_string (stream, component->goad_id,
					   TRUE, ev);
	
	if (ev->_major != CORBA_NO_EXCEPTION)
		g_warning ("Error saving object_id '%s'", component->goad_id);
}
