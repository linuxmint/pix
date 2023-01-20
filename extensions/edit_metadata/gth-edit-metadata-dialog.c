/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "gth-edit-metadata-dialog.h"


G_DEFINE_INTERFACE (GthEditMetadataDialog, gth_edit_metadata_dialog, 0)


static void
gth_edit_metadata_dialog_default_init (GthEditMetadataDialogInterface *iface)
{
	/* void */
}


void
gth_edit_metadata_dialog_set_file_list (GthEditMetadataDialog *self,
					GList                 *file_list)
{
	GTH_EDIT_METADATA_DIALOG_GET_INTERFACE (self)->set_file_list (self, file_list);
}


void
gth_edit_metadata_dialog_update_info (GthEditMetadataDialog *self,
				      GList                 *file_list)
{
	GTH_EDIT_METADATA_DIALOG_GET_INTERFACE (self)->update_info (self, file_list);
}

