/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include "gth-property-view.h"


G_DEFINE_INTERFACE (GthPropertyView, gth_property_view, 0)


static void
gth_property_view_default_init (GthPropertyViewInterface *iface)
{
	/* void */
}


const char *
gth_property_view_get_name (GthPropertyView *self)
{
	return GTH_PROPERTY_VIEW_GET_INTERFACE (self)->get_name (self);
}


const char *
gth_property_view_get_icon (GthPropertyView *self)
{
	return GTH_PROPERTY_VIEW_GET_INTERFACE (self)->get_icon (self);
}


gboolean
gth_property_view_can_view (GthPropertyView *self,
			    GthFileData     *file_data)
{
	return GTH_PROPERTY_VIEW_GET_INTERFACE (self)->can_view (self, file_data);
}


void
gth_property_view_set_file (GthPropertyView *self,
			    GthFileData     *file_data)
{
	GTH_PROPERTY_VIEW_GET_INTERFACE (self)->set_file (self, file_data);
}
