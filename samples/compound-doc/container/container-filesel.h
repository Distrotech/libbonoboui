/* $Id$ */
/*
  Sample-Container Copyright (C) 2000 �RDI Gerg� <cactus@cactus.rulez.org>
  
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

#ifndef SAMPLE_CONTAINER_FILESEL_H
#define SAMPLE_CONTAINER_FILESEL_H

#include "container.h"

void container_request_file (SampleApp *app,
			     gboolean save,
			     GtkSignalFunc cb,
			     gpointer user_data);
#endif
