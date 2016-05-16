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
#include "utils.h"


static gboolean
remove_tag_if_not_present (gpointer key,
                	   gpointer value,
                	   gpointer user_data)
{
	GthStringList *file_tags = user_data;

	return g_list_find_custom (gth_string_list_get_list (file_tags), key, (GCompareFunc) g_strcmp0) == NULL;
}


void
utils_get_common_tags (GList       *file_list, /* GthFileData list */
		       GHashTable **common_tags_out,
		       GHashTable **other_tags_out)
{
	GHashTable *all_tags;
	GHashTable *common_tags;
	GList      *scan;
	GHashTable *no_common_tags;
	GList      *all_tags_list;
	GList      *scan_tags;

	all_tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	common_tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	for (scan = file_list; scan; scan = scan->next) {
		GthFileData   *file_data = scan->data;
		GthMetadata   *metadata;
		GthStringList *file_tags;

		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::tags");
		file_tags = gth_metadata_get_string_list (metadata);
		if (file_tags != NULL) {
			for (scan_tags = gth_string_list_get_list (file_tags);
			     scan_tags != NULL;
			     scan_tags = scan_tags->next)
			{
				char *tag = scan_tags->data;

				/* update the all tags set */

				if (g_hash_table_lookup (all_tags, tag) == NULL)
					g_hash_table_insert (all_tags, g_strdup (tag), GINT_TO_POINTER (1));

				/* update the common tags set */

				if (scan == file_list)
					g_hash_table_insert (common_tags, g_strdup (tag), GINT_TO_POINTER (1));
				else
					g_hash_table_foreach_remove (common_tags, remove_tag_if_not_present, file_tags);
			}
		}
		else
			g_hash_table_remove_all (common_tags);
	}

	no_common_tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	all_tags_list = g_hash_table_get_keys (all_tags);
	for (scan_tags = all_tags_list; scan_tags; scan_tags = scan_tags->next) {
		char *tag = scan_tags->data;

		if (g_hash_table_lookup (common_tags, tag) == NULL)
			g_hash_table_insert (no_common_tags, g_strdup (tag), GINT_TO_POINTER (1));
	}

	if (common_tags_out != NULL)
		*common_tags_out = g_hash_table_ref (common_tags);

	if (other_tags_out != NULL)
		*other_tags_out = g_hash_table_ref (no_common_tags);

	g_list_free (all_tags_list);
	g_hash_table_unref (no_common_tags);
	g_hash_table_unref (common_tags);
	g_hash_table_unref (all_tags);
}
