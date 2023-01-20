/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "glib-utils.h"
#include "gth-file-details.h"
#include "gth-property-view.h"


static void gth_file_details_gth_property_view_interface_init (GthPropertyViewInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthFileDetails,
			 gth_file_details,
			 GTH_TYPE_FILE_PROPERTIES,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_PROPERTY_VIEW,
					 	gth_file_details_gth_property_view_interface_init))


static const char *
gth_file_details_real_get_name (GthPropertyView *self)
{
	return _("Metadata");
}


static const char *
gth_file_details_real_get_icon (GthPropertyView *self)
{
	return "tag-symbolic";
}


static void
gth_file_details_class_init (GthFileDetailsClass *klass)
{
	/* void */
}


static void
gth_file_details_init (GthFileDetails *self)
{
	g_object_set (self, "show-details", TRUE, NULL);
}


static void
gth_file_details_gth_property_view_interface_init (GthPropertyViewInterface *iface)
{
	iface->get_name = gth_file_details_real_get_name;
	iface->get_icon = gth_file_details_real_get_icon;
}
