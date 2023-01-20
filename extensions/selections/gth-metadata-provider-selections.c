/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <glib.h>
#include <gthumb.h>
#include "gth-metadata-provider-selections.h"
#include "gth-selections-manager.h"


G_DEFINE_TYPE (GthMetadataProviderSelections, gth_metadata_provider_selections, GTH_TYPE_METADATA_PROVIDER)


static gboolean
gth_metadata_provider_selections_can_read (GthMetadataProvider  *self,
					   GthFileData          *file_data,
					   const char           *mime_type,
					   char                **attribute_v)
{
	return _g_file_attributes_matches_any_v (GTH_FILE_ATTRIBUTE_EMBLEMS,
						 attribute_v);
}


static gboolean
gth_metadata_provider_selections_can_write (GthMetadataProvider  *self,
				            const char           *mime_type,
				            char                **attribute_v)
{
	return FALSE;
}


static void
gth_metadata_provider_selections_read (GthMetadataProvider *self,
				       GthFileData         *file_data,
				       const char          *attributes,
				       GCancellable        *cancellable)
{
	GList         *emblem_list;
	GthStringList *emblems;
	GthStringList *other_emblems;
	int            i;

	emblem_list = NULL;
	for (i = GTH_SELECTIONS_MANAGER_N_SELECTIONS; i >= 0; i--) {
		if (gth_selections_manager_file_exists (i, file_data->file))
			emblem_list = g_list_prepend (emblem_list, g_strdup (gth_selection_get_icon_name (i)));
	}

	emblems = gth_string_list_new (emblem_list);
	other_emblems = (GthStringList *) g_file_info_get_attribute_object (file_data->info, GTH_FILE_ATTRIBUTE_EMBLEMS);
	if (other_emblems != NULL)
		gth_string_list_append (emblems, other_emblems);

	g_file_info_set_attribute_object (file_data->info, GTH_FILE_ATTRIBUTE_EMBLEMS, G_OBJECT (emblems));

	g_object_unref (emblems);
	_g_string_list_free (emblem_list);
}


static void
gth_metadata_provider_selections_write (GthMetadataProvider   *self,
				        GthMetadataWriteFlags  flags,
				        GthFileData           *file_data,
				        const char            *attributes,
				        GCancellable          *cancellable)
{
	/* void: never called */
}


static void
gth_metadata_provider_selections_class_init (GthMetadataProviderSelectionsClass *klass)
{
	GthMetadataProviderClass *mp_class;

	mp_class = GTH_METADATA_PROVIDER_CLASS (klass);
	mp_class->can_read = gth_metadata_provider_selections_can_read;
	mp_class->can_write = gth_metadata_provider_selections_can_write;
	mp_class->read = gth_metadata_provider_selections_read;
	mp_class->write = gth_metadata_provider_selections_write;
}


static void
gth_metadata_provider_selections_init (GthMetadataProviderSelections *self)
{
	/* void */
}
