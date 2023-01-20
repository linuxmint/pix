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
#include <string.h>
#include "dom.h"
#include "glib-utils.h"
#include "gth-duplicable.h"
#include "gth-filter.h"
#include "gth-filter-file.h"
#include "gth-main.h"


#define FILTER_FORMAT "1.0"


struct _GthFilterFile
{
	GList *items;
};


GthFilterFile *
gth_filter_file_new (void)
{
	GthFilterFile *filters;

	filters = g_new0 (GthFilterFile, 1);
  	filters->items = NULL;

  	return filters;
}


void
gth_filter_file_free (GthFilterFile *filters)
{
	if (filters == NULL)
		return;
	_g_object_list_unref (filters->items);
	g_free (filters);
}


gboolean
gth_filter_file_load_from_data (GthFilterFile  *filters,
                                const char     *data,
                                gsize           length,
                                GError        **error)
{
	DomDocument *doc;
	gboolean     success;

	doc = dom_document_new ();
	success = dom_document_load (doc, data, length, error);
	if (success) {
		DomElement *filters_node;
		DomElement *child;
		GList      *new_items = NULL;

		filters_node = DOM_ELEMENT (doc)->first_child;
		if ((filters_node != NULL) && (g_strcmp0 (filters_node->tag_name, "filters") == 0)) {
			for (child = filters_node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				GthTest *test = NULL;

				if (strcmp (child->tag_name, "filter") == 0) {
					test = (GthTest*) gth_filter_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (test), child);
				}
				else if (strcmp (child->tag_name, "test") == 0) {
					test = gth_main_get_registered_object (GTH_TYPE_TEST, dom_element_get_attribute (child, "id"));
					if (test != NULL)
						dom_domizable_load_from_element (DOM_DOMIZABLE (test), child);
				}

				if (test == NULL)
					continue;

				new_items = g_list_prepend (new_items, test);
			}

			new_items = g_list_reverse (new_items);
			filters->items = g_list_concat (filters->items, new_items);
		}
	}
	g_object_unref (doc);

	return success;
}


gboolean
gth_filter_file_load_from_file (GthFilterFile  *filters,
				GFile          *file,
                                GError        **error)
{
	char     *buffer;
	gsize     len;
	GError   *read_error;
	gboolean  retval;

	g_return_val_if_fail (filters != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	read_error = NULL;
	if (! _g_file_load_in_buffer (file, (void **) &buffer, &len, NULL, &read_error)) {
		g_propagate_error (error, read_error);
		return FALSE;
	}

	read_error = NULL;
	retval = gth_filter_file_load_from_data (filters,
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
gth_filter_file_to_data (GthFilterFile  *filters,
			 gsize          *len,
			 GError        **data_error)
{
	DomDocument *doc;
	DomElement  *root;
	char        *data;
	GList       *scan;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "filters",
					    "version", FILTER_FORMAT,
					    NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = filters->items; scan; scan = scan->next)
		dom_element_append_child (root, dom_domizable_create_element (DOM_DOMIZABLE (scan->data), doc));
	data = dom_document_dump (doc, len);

	g_object_unref (doc);

	return data;
}


gboolean
gth_filter_file_to_file (GthFilterFile  *filters,
                         GFile          *file,
                         GError        **error)
{
	char     *data;
	GError   *data_error, *write_error;
	gsize     len;
	gboolean  retval;

	g_return_val_if_fail (filters != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	data_error = NULL;
	data = gth_filter_file_to_data (filters, &len, &data_error);
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


GList *
gth_filter_file_get_tests (GthFilterFile *filters)
{
	GList *list = NULL;
	GList *scan;

	for (scan = filters->items; scan; scan = scan->next)
		list = g_list_prepend (list, gth_duplicable_duplicate (GTH_DUPLICABLE (scan->data)));

	return g_list_reverse (list);
}


static int
find_by_id (gconstpointer a,
            gconstpointer b)
{
	GthTest *test = (GthTest *) a;
	GthTest *test_to_find = (GthTest *) b;

	return g_strcmp0 (gth_test_get_id (test), gth_test_get_id (test_to_find));
}


gboolean
gth_filter_file_has_test (GthFilterFile *filters,
			  GthTest       *test)
{
	return g_list_find_custom (filters->items, test, find_by_id) != NULL;
}


void
gth_filter_file_add (GthFilterFile *filters,
		     GthTest       *test)
{
	GList *link;

	g_object_ref (test);

	link = g_list_find_custom (filters->items, test, find_by_id);
	if (link != NULL) {
		g_object_unref (link->data);
		link->data = test;
	}
	else
		filters->items = g_list_append (filters->items, test);
}


void
gth_filter_file_remove (GthFilterFile *filters,
			GthTest       *test)
{
	GList *link;

	link = g_list_find_custom (filters->items, test, find_by_id);
	if (link == NULL)
		return;
	filters->items = g_list_remove_link (filters->items, link);
	_g_object_list_unref (link);
}


void
gth_filter_file_clear (GthFilterFile *filters)
{
	_g_object_list_unref (filters->items);
	filters->items = NULL;
}
