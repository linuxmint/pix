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
#include <string.h>
#include <glib/gi18n.h>
#include "dom.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-tags-file.h"


#define FILE_FORMAT "1.0"


struct _GthTagsFile
{
	GList  *items;
	char  **tags;
};


GthTagsFile *
gth_tags_file_new (void)
{
	GthTagsFile *tags;

	tags = g_new0 (GthTagsFile, 1);
  	tags->items = NULL;
  	tags->tags = NULL;

  	return tags;
}


void
gth_tags_file_free (GthTagsFile *tags)
{
	if (tags == NULL)
		return;
	_g_string_list_free (tags->items);
	g_strfreev (tags->tags);
	g_free (tags);
}


gboolean
gth_tags_file_load_from_data (GthTagsFile  *tags,
                              const char   *data,
                              gsize         length,
                              GError      **error)
{
	DomDocument *doc;
	gboolean     success;

	_g_string_list_free (tags->items);
	tags->items = NULL;

	doc = dom_document_new ();
	success = dom_document_load (doc, data, length, error);
	if (success) {
		DomElement *tags_node;
		DomElement *child;

		tags_node = DOM_ELEMENT (doc)->first_child;
		if ((tags_node != NULL) && (g_strcmp0 (tags_node->tag_name, "tags") == 0)) {
			for (child = tags_node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				if (strcmp (child->tag_name, "tag") == 0) {
					const char *tag_value;

					tag_value = dom_element_get_attribute (child, "value");
					if (tag_value != NULL)
						tags->items = g_list_prepend (tags->items, g_strdup (tag_value));
				}
			}

			tags->items = g_list_reverse (tags->items);
		}
	}

	g_object_unref (doc);

	return success;
}


gboolean
gth_tags_file_load_from_file (GthTagsFile  *tags,
                              GFile        *file,
                              GError      **error)
{
	char     *buffer;
	gsize     len;
	GError   *read_error;
	gboolean  retval;

	g_return_val_if_fail (tags != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	read_error = NULL;
	if (! _g_file_load_in_buffer (file, (void **) &buffer, &len, NULL, &read_error)) {
		g_propagate_error (error, read_error);
		return FALSE;
	}

	read_error = NULL;
	retval = gth_tags_file_load_from_data (tags,
					       buffer,
					       len,
					       &read_error);
  	if (read_error != NULL) {
		g_propagate_error (error, read_error);
		g_free (buffer);
		return FALSE;
	}

  	g_free (buffer);

	return retval;
}


char *
gth_tags_file_to_data (GthTagsFile  *tags,
		       gsize        *len,
		       GError      **data_error)
{
	DomDocument *doc;
	DomElement  *root;
	char        *data;
	GList       *scan;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "tags",
					    "version", FILE_FORMAT,
					    NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = tags->items; scan; scan = scan->next) {
		const char *tag_value = scan->data;
		dom_element_append_child (root, dom_document_create_element (doc, "tag", "value", tag_value, NULL));
	}
	data = dom_document_dump (doc, len);

	g_object_unref (doc);

	return data;
}


gboolean
gth_tags_file_to_file (GthTagsFile  *tags,
                       GFile        *file,
                       GError      **error)
{
	char     *data;
	GError   *data_error, *write_error;
	gsize     len;
	gboolean  retval;

	g_return_val_if_fail (tags != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	data_error = NULL;
	data = gth_tags_file_to_data (tags, &len, &data_error);
	if (data_error) {
		g_propagate_error (error, data_error);
		return FALSE;
	}

	write_error = NULL;
	if (! _g_file_write (file, FALSE, 0, data, len, NULL, &write_error)) {
		g_propagate_error (error, write_error);
		retval = FALSE;
	}
	else
		retval = TRUE;

	g_free (data);

	return retval;
}


static int
sort_tag_by_name (gconstpointer a,
		  gconstpointer b,
		  gpointer      user_data)
{
	char *sa = * (char **) a;
	char *sb = * (char **) b;

	return g_utf8_collate (sa, sb);
}


char **
gth_tags_file_get_tags (GthTagsFile *tags)
{
	GList *scan;
	int    i;

	if (tags->tags != NULL) {
		g_strfreev (tags->tags);
		tags->tags = NULL;
	}

	if (g_list_length (tags->items) > 0) {
		tags->tags = g_new (char *, g_list_length (tags->items) + 1);
		for (i = 0, scan = tags->items; scan; scan = scan->next)
			tags->tags[i++] = g_strdup ((char *) scan->data);
		tags->tags[i] = NULL;
	}
	else {
		char *default_tags[] = { N_("Holidays"),
					 N_("Temporary"),
					 N_("Screenshots"),
					 N_("Science"),
					 N_("Favorite"),
					 N_("Important"),
					 N_("GNOME"),
					 N_("Games"),
					 N_("Party"),
					 N_("Birthday"),
					 N_("Astronomy"),
					 N_("Family"),
					 NULL };

		tags->tags = g_new (char *, g_strv_length (default_tags) + 1);
		for (i = 0; default_tags[i] != NULL; i++)
			tags->tags[i] = g_strdup (_(default_tags[i]));
		tags->tags[i] = NULL;
	}

	g_qsort_with_data (tags->tags,
			   g_strv_length (tags->tags),
			   sizeof (char *),
			   sort_tag_by_name,
			   NULL);

	return tags->tags;
}


gboolean
gth_tags_file_has_tag (GthTagsFile *tags,
		       const char  *tag)
{
	return g_list_find_custom (tags->items, tag, (GCompareFunc) g_utf8_collate) != NULL;
}


gboolean
gth_tags_file_add (GthTagsFile *tags,
		   const char  *tag)
{
	GList *link;

	link = g_list_find_custom (tags->items, tag, (GCompareFunc) g_utf8_collate);
	if (link == NULL) {
		tags->items = g_list_append (tags->items, g_strdup (tag));
		return TRUE;
	}

	return FALSE;
}


gboolean
gth_tags_file_remove (GthTagsFile *tags,
		      const char  *tag)
{
	GList *link;

	link = g_list_find_custom (tags->items, tag, (GCompareFunc) g_utf8_collate);
	if (link == NULL)
		return FALSE;

	tags->items = g_list_remove_link (tags->items, link);
	_g_string_list_free (link);

	return TRUE;
}


void
gth_tags_file_clear (GthTagsFile *tags)
{
	_g_string_list_free (tags->items);
	tags->items = NULL;
}
