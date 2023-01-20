/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "gth-duplicable.h"


G_DEFINE_INTERFACE (GthDuplicable, gth_duplicable, 0)


static void
gth_duplicable_default_init (GthDuplicableInterface *iface)
{
	/* void */
}


GObject *
gth_duplicable_duplicate (GthDuplicable *self)
{
	return GTH_DUPLICABLE_GET_INTERFACE (self)->duplicate (self);
}
