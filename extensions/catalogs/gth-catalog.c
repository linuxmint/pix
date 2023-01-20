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
#include <glib.h>
#include <gthumb.h>
#include "gth-catalog.h"


#define CATALOG_FORMAT "1.0"


struct _GthCatalogPrivate {
	GthCatalogType  type;
	GFile          *file;
	GList          *file_list;
	GHashTable     *file_hash;  /* used to avoid duplicates */
	char           *name;
	GthDateTime    *date_time;
	gboolean        active;
	char           *order;
	gboolean        order_inverse;
};


G_DEFINE_TYPE_WITH_CODE (GthCatalog,
			 gth_catalog,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthCatalog))


static void
gth_catalog_finalize (GObject *object)
{
	GthCatalog *catalog = GTH_CATALOG (object);

	g_value_hash_unref (catalog->attributes);

	if (catalog->priv->file != NULL)
		g_object_unref (catalog->priv->file);
	g_free (catalog->priv->name);
	_g_object_list_unref (catalog->priv->file_list);
	g_hash_table_destroy (catalog->priv->file_hash);
	gth_datetime_free (catalog->priv->date_time);
	g_free (catalog->priv->order);

	G_OBJECT_CLASS (gth_catalog_parent_class)->finalize (object);
}


static DomElement *
base_create_root (GthCatalog  *catalog,
		  DomDocument *doc)
{
	return dom_document_create_element (doc, "catalog",
					    "version", CATALOG_FORMAT,
					    NULL);
}


static void
base_read_from_doc (GthCatalog *catalog,
		    DomElement *root)
{
	GList      *file_list;
	DomElement *child;

	file_list = NULL;
	for (child = root->first_child; child; child = child->next_sibling) {
		if (g_strcmp0 (child->tag_name, "files") == 0) {
			DomElement *file;

			for (file = child->first_child; file; file = file->next_sibling) {
				const char *uri;

				uri = dom_element_get_attribute (file, "uri");
				if (uri != NULL)
					file_list = g_list_prepend (file_list, g_file_new_for_uri (uri));
			}
			file_list = g_list_reverse (file_list);
		}
		if (g_strcmp0 (child->tag_name, "order") == 0)
			gth_catalog_set_order (catalog,
					       dom_element_get_attribute (child, "type"),
					       g_strcmp0 (dom_element_get_attribute (child, "inverse"), "1") == 0);
		if (g_strcmp0 (child->tag_name, "date") == 0)
			gth_datetime_from_exif_date (catalog->priv->date_time, dom_element_get_inner_text (child));
		if (g_strcmp0 (child->tag_name, "name") == 0)
			gth_catalog_set_name (catalog, dom_element_get_inner_text (child));
	}
	gth_catalog_set_file_list (catalog, file_list);

	gth_hook_invoke ("gth-catalog-read-from-doc", catalog, root);

	_g_object_list_unref (file_list);
}


static void
read_catalog_data_from_xml (GthCatalog  *catalog,
		   	    const char  *buffer,
		   	    gsize        count,
		   	    GError     **error)
{
	DomDocument *doc;

	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, count, error))
		GTH_CATALOG_GET_CLASS (catalog)->read_from_doc (catalog, DOM_ELEMENT (doc)->first_child);

	g_object_unref (doc);
}


static void
read_catalog_data_old_format (GthCatalog *catalog,
			      const char *buffer,
			      gsize       count)
{
	GInputStream     *mem_stream;
	GDataInputStream *data_stream;
	gboolean          is_search;
	int               list_start;
	int               n_line;
	char             *line;

	mem_stream = g_memory_input_stream_new_from_data (buffer, count, NULL);
	data_stream = g_data_input_stream_new (mem_stream);

	is_search = (strncmp (buffer, "# Search", 8) == 0);
	if (is_search)
		list_start = 10;
	else
		list_start = 1;

	gth_catalog_set_file_list (catalog, NULL);

	n_line = 0;
	while ((line = g_data_input_stream_read_line (data_stream, NULL, NULL, NULL)) != NULL) {
		n_line++;

		if (is_search) {
			/* FIXME: read the search metadata here. */
		}

		if (n_line > list_start) {
			char *uri;

			uri = g_strndup (line + 1, strlen (line) - 2);
			catalog->priv->file_list = g_list_prepend (catalog->priv->file_list, g_file_new_for_uri (uri));

			g_free (uri);
		}
		g_free (line);
	}

	catalog->priv->file_list = g_list_reverse (catalog->priv->file_list);

	g_object_unref (data_stream);
	g_object_unref (mem_stream);
}


static void
base_write_to_doc (GthCatalog  *catalog,
		   DomDocument *doc,
		   DomElement  *root)
{
	if (catalog->priv->name != NULL)
		dom_element_append_child (root, dom_document_create_element_with_text (doc, catalog->priv->name, "name", NULL));

	if (gth_datetime_valid_date (catalog->priv->date_time)) {
		char *s;

		s = gth_datetime_to_exif_date (catalog->priv->date_time);
		dom_element_append_child (root, dom_document_create_element_with_text (doc, s, "date", NULL));
		g_free (s);
	}

	if (catalog->priv->order != NULL)
		dom_element_append_child (root, dom_document_create_element (doc, "order",
									     "type", catalog->priv->order,
									     "inverse", (catalog->priv->order_inverse ? "1" : "0"),
									     NULL));

	if (catalog->priv->file_list != NULL) {
		DomElement *node;
		GList      *scan;

		node = dom_document_create_element (doc, "files", NULL);
		dom_element_append_child (root, node);

		for (scan = catalog->priv->file_list; scan; scan = scan->next) {
			GFile *file = scan->data;
			char  *uri;

			uri = g_file_get_uri (file);
			dom_element_append_child (node, dom_document_create_element (doc, "file", "uri", uri, NULL));

			g_free (uri);
		}
	}

	gth_hook_invoke ("gth-catalog-write-to-doc", catalog, doc, root);
}


static void
gth_catalog_class_init (GthCatalogClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) class;
	object_class->finalize = gth_catalog_finalize;

	class->create_root = base_create_root;
	class->read_from_doc = base_read_from_doc;
	class->write_to_doc = base_write_to_doc;
}


static void
gth_catalog_init (GthCatalog *catalog)
{
	catalog->attributes = g_value_hash_new ();
	catalog->priv = gth_catalog_get_instance_private (catalog);
	catalog->priv->type = GTH_CATALOG_TYPE_INVALID;
	catalog->priv->file = NULL;
	catalog->priv->file_list = NULL;
	catalog->priv->file_hash = g_hash_table_new_full (g_file_hash, (GEqualFunc) g_file_equal, NULL, NULL);
	catalog->priv->name = NULL;
	catalog->priv->date_time = gth_datetime_new ();
	catalog->priv->order = NULL;
	catalog->priv->order_inverse = FALSE;
}


GthCatalog *
gth_catalog_new (void)
{
	return (GthCatalog *) g_object_new (GTH_TYPE_CATALOG, NULL);
}


GthCatalog *
gth_catalog_new_for_file (GFile *file)
{
	char       *uri;
	GthCatalog *catalog;

	if (file == NULL)
		return NULL;

	uri = g_file_get_uri (file);
	catalog = gth_hook_invoke_get ("gth-catalog-new-for-uri", uri);

	g_free (uri);

	return catalog;
}


GthCatalog *
gth_catalog_new_from_data (const void  *buffer,
			   gsize        count,
			   GError     **error)
{
	char       *text_buffer;
	GthCatalog *catalog = NULL;

	text_buffer = (char *) buffer;
	if ((text_buffer == NULL) || (*text_buffer == 0))
		return NULL;

	if (strncmp (text_buffer, "<?xml ", 6) == 0) {
		catalog = gth_hook_invoke_get ("gth-catalog-load-from-data", (gpointer) buffer);
		if (catalog != NULL)
			read_catalog_data_from_xml (catalog, text_buffer, count, error);
		else
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
	}
	else {
		catalog = gth_catalog_new ();
		read_catalog_data_old_format (catalog, text_buffer, count);
	}

	return catalog;
}


void
gth_catalog_set_file (GthCatalog *catalog,
		      GFile      *file)
{
	if (catalog->priv->file != NULL) {
		g_object_unref (catalog->priv->file);
		catalog->priv->file = NULL;
	}

	if (file != NULL)
		catalog->priv->file = g_file_dup (file);

	catalog->priv->type = GTH_CATALOG_TYPE_CATALOG;
}


GFile *
gth_catalog_get_file (GthCatalog *catalog)
{
	return catalog->priv->file;
}


void
gth_catalog_set_name (GthCatalog *catalog,
		      const char *name)
{
	g_free (catalog->priv->name);
	catalog->priv->name = NULL;
	if ((name != NULL) && (strcmp (name, "") != 0))
		catalog->priv->name = g_strdup (name);;
}


const char *
gth_catalog_get_name (GthCatalog *catalog)
{
	return catalog->priv->name;
}


void
gth_catalog_set_date (GthCatalog  *catalog,
		      GthDateTime *date_time)
{
	if (g_date_valid (date_time->date))
		g_date_set_dmy (catalog->priv->date_time->date,
				g_date_get_day (date_time->date),
				g_date_get_month (date_time->date),
				g_date_get_year (date_time->date));
	else
		g_date_clear (catalog->priv->date_time->date, 1);
	gth_time_set_hms (catalog->priv->date_time->time, 0, 0, 0, 0);
}


GthDateTime *
gth_catalog_get_date (GthCatalog *catalog)
{
	return catalog->priv->date_time;
}


void
gth_catalog_set_order (GthCatalog *catalog,
		       const char *order,
		       gboolean    inverse)
{
	g_free (catalog->priv->order);
	catalog->priv->order = NULL;

	if (order != NULL)
		catalog->priv->order = g_strdup (order);
	catalog->priv->order_inverse = inverse;
}


const char *
gth_catalog_get_order (GthCatalog *catalog,
		       gboolean   *inverse)
{
	*inverse = catalog->priv->order_inverse;
	return catalog->priv->order;
}


char *
gth_catalog_to_data (GthCatalog *catalog,
		     gsize      *length)
{
	DomDocument *doc;
	DomElement  *root;
	char        *data;

	doc = dom_document_new ();
	root = GTH_CATALOG_GET_CLASS (catalog)->create_root (catalog, doc);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	GTH_CATALOG_GET_CLASS (catalog)->write_to_doc (catalog, doc, root);
	data = dom_document_dump (doc, length);

	g_object_unref (doc);

	return data;
}


void
gth_catalog_set_file_list (GthCatalog *catalog,
			   GList      *file_list)
{
	_g_object_list_unref (catalog->priv->file_list);
	catalog->priv->file_list = NULL;
	g_hash_table_remove_all (catalog->priv->file_hash);

	if (file_list != NULL) {
		GList *list;
		GList *scan;

		list = NULL;
		for (scan = file_list; scan; scan = scan->next) {
			GFile *file = scan->data;

			if (g_hash_table_lookup (catalog->priv->file_hash, file) != NULL)
				continue;
			file = g_file_dup (file);
			list = g_list_prepend (list, file);
			g_hash_table_insert (catalog->priv->file_hash, file, GINT_TO_POINTER (1));
		}
		catalog->priv->file_list = g_list_reverse (list);
	}
}


GList *
gth_catalog_get_file_list (GthCatalog *catalog)
{
	return catalog->priv->file_list;
}


gboolean
gth_catalog_insert_file (GthCatalog *catalog,
			 GFile      *file,
			 int         pos)
{
	if (g_hash_table_lookup (catalog->priv->file_hash, file) != NULL)
		return FALSE;

	file = g_file_dup (file);
	catalog->priv->file_list = g_list_insert (catalog->priv->file_list, file, pos);
	g_hash_table_insert (catalog->priv->file_hash, file, GINT_TO_POINTER (1));

	return TRUE;
}


int
gth_catalog_remove_file (GthCatalog *catalog,
			 GFile      *file)
{
	GList *scan;
	int    i = 0;

	g_return_val_if_fail (catalog != NULL, -1);
	g_return_val_if_fail (file != NULL, -1);

	for (scan = catalog->priv->file_list; scan; scan = scan->next, i++)
		if (g_file_equal ((GFile *) scan->data, file))
			break;

	if (scan == NULL)
		return -1;

	catalog->priv->file_list = g_list_remove_link (catalog->priv->file_list, scan);
	g_hash_table_remove (catalog->priv->file_hash, file);

	_g_object_list_unref (scan);

	return i;
}


static char *
get_display_name (GFile       *file,
		  const char  *name,
		  GthDateTime *date_time)
{
	GString *display_name;
	char    *basename;

	display_name = g_string_new ("");
	basename = g_file_get_basename (file);
	if ((basename == NULL) || (strcmp (basename, "/") == 0)) {
		 g_string_append (display_name, _("Catalogs"));
	}
	else {
		if ((name == NULL) && ! gth_datetime_valid_date (date_time)) {
			char *name;
			char *utf8_name;

			name = _g_path_remove_extension (basename);
			utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
			g_string_append (display_name, utf8_name);

			g_free (utf8_name);
			g_free (name);
		}
		else {
			if (name != NULL)
				g_string_append (display_name, name);

			if (gth_datetime_valid_date (date_time)) {
				char *formatted;

				formatted = gth_datetime_strftime (date_time, "%x");
				if ((name == NULL) || (strstr (name, formatted) == NULL)) {
					if (name != NULL)
						g_string_append (display_name, " (");
					g_string_append (display_name, formatted);
					if (name != NULL)
						g_string_append (display_name, ")");
				}
				g_free (formatted);
			}
		}
	}

	g_free (basename);

	return g_string_free (display_name, FALSE);
}


static char *
get_edit_name (GFile       *file,
	       const char  *name,
	       GthDateTime *date_time)
{
	GString *display_name;
	char    *basename;

	display_name = g_string_new ("");
	basename = g_file_get_basename (file);
	if ((basename == NULL) || (strcmp (basename, "/") == 0)) {
		 g_string_append (display_name, _("Catalogs"));
	}
	else {
		if (name == NULL) {
			char *name;
			char *utf8_name;

			name = _g_path_remove_extension (basename);
			utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
			g_string_append (display_name, utf8_name);

			g_free (utf8_name);
			g_free (name);
		}
		else
			g_string_append (display_name, name);
	}

	g_free (basename);

	return g_string_free (display_name, FALSE);
}


static void
update_standard_attributes (GFile       *file,
			    GFileInfo   *info,
			    const char  *name,
			    GthDateTime *date_time)
{
	char *display_name;
	char *edit_name;

	if (gth_datetime_valid_date (date_time)) {
		char *sort_order_s;

		sort_order_s = gth_datetime_strftime (date_time, "%Y%m%d");
		_g_file_info_set_secondary_sort_order (info, atoi (sort_order_s));

		g_free (sort_order_s);
	}
	else
		g_file_info_remove_attribute (info, "gth::standard::secondary-sort-order");

	display_name = get_display_name (file, name, date_time);
	if (display_name != NULL) {
		g_file_info_set_display_name (info, display_name);
		g_free (display_name);
	}

	edit_name = get_edit_name (file, name, date_time);
	if (edit_name != NULL) {
		g_file_info_set_edit_name (info, edit_name);
		g_free (edit_name);
	}
}


void
gth_catalog_update_metadata (GthCatalog  *catalog,
			     GthFileData *file_data)
{
	const char *sort_type;
	gboolean    sort_inverse;

	/* sort::type,sort::inverse */

	sort_type = gth_catalog_get_order (catalog, &sort_inverse);
	if (sort_type != NULL) {
		g_file_info_set_attribute_string (file_data->info, "sort::type", sort_type);
		g_file_info_set_attribute_boolean (file_data->info, "sort::inverse", sort_inverse);
	}
	else {
		g_file_info_remove_attribute (file_data->info, "sort::type");
		g_file_info_remove_attribute (file_data->info, "sort::inverse");
	}

	/* general::event-date */

	if (gth_datetime_valid_date (catalog->priv->date_time)) {
		GObject *metadata;
		char    *raw;
		char    *formatted;
		char    *sort_order_s;

		metadata = (GObject *) gth_metadata_new ();
		raw = gth_datetime_to_exif_date (catalog->priv->date_time);
		formatted = gth_datetime_strftime (catalog->priv->date_time, "%x");
		g_object_set (metadata,
			      "id", "general::event-date",
			      "raw", raw,
			      "formatted", formatted,
			      NULL);
		g_file_info_set_attribute_object (file_data->info, "general::event-date", metadata);

		sort_order_s = gth_datetime_strftime (catalog->priv->date_time, "%Y%m%d");
		_g_file_info_set_secondary_sort_order (file_data->info, atoi (sort_order_s));

		g_free (formatted);
		g_free (raw);
		g_object_unref (metadata);
	}
	else {
		g_file_info_remove_attribute (file_data->info, "general::event-date");
		g_file_info_remove_attribute (file_data->info, "gth::standard::secondary-sort-order");
	}

	/* standard::display-name,standard::sort-order */

	update_standard_attributes (file_data->file,
				    file_data->info,
				    catalog->priv->name,
				    catalog->priv->date_time);

	gth_hook_invoke ("gth-catalog-write-metadata", catalog, file_data);
}


int
gth_catalog_get_size (GthCatalog *catalog)
{
	return g_hash_table_size (catalog->priv->file_hash);
}


/* utils */


GFile *
gth_catalog_get_base (void)
{
	return gth_user_dir_get_file_for_read (GTH_DIR_DATA, GTHUMB_DIR, "catalogs", NULL);
}


GFile *
gth_catalog_file_to_gio_file (GFile *file)
{
	GFile      *gio_file = NULL;
	char       *uri;
	UriParts    file_parts;

	if (! g_file_has_uri_scheme (file, "catalog"))
		return g_file_dup (file);

	uri = g_file_get_uri (file);
	if (! _g_uri_split (uri, &file_parts))
		return NULL;

	if (file_parts.query != NULL) {
		char *new_uri;

		new_uri = g_uri_unescape_string (file_parts.query, NULL);
		gio_file = g_file_new_for_uri (new_uri);

		g_free (new_uri);
	}
	else {
		GFile *base;
		char  *base_uri;
		char  *new_uri;

		base = gth_catalog_get_base ();
		base_uri = g_file_get_uri (base);
		new_uri = _g_uri_append_path (base_uri, file_parts.path);
		gio_file = g_file_new_for_uri (new_uri);

		g_free (new_uri);
		g_free (base_uri);
		g_object_unref (base);
	}

	_g_uri_parts_clear (&file_parts);
	g_free (uri);

	return gio_file;
}


GFile *
gth_catalog_file_from_gio_file (GFile *gio_file,
				GFile *catalog)
{
	GFile *gio_base;
	GFile *file = NULL;
	char  *path;

	gio_base = gth_catalog_get_base ();
	if (g_file_equal (gio_base, gio_file)) {
		g_object_unref (gio_base);
		return g_file_new_for_uri ("catalog:///");
	}

	path = g_file_get_relative_path (gio_base, gio_file);
	if (path != NULL) {
		GFile *base;

		base = g_file_new_for_uri ("catalog:///");
		file = _g_file_append_path (base, path);

		g_object_unref (base);
	}
	else if (catalog != NULL) {
		char *catalog_uri;
		char *file_uri;
		char *query;
		char *uri;

		catalog_uri = g_file_get_uri (catalog);
		file_uri = g_file_get_uri (gio_file);
		query = g_uri_escape_string (file_uri, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
		uri = g_strconcat (file_uri, "?", query, NULL);
		file = g_file_new_for_uri (uri);

		g_free (uri);
		g_free (query);
		g_free (file_uri);
		g_free (catalog_uri);
	}

	g_free (path);
	g_object_unref (gio_base);

	return file;
}


GFile *
gth_catalog_file_from_relative_path (const char *name,
				     const char *file_extension)
{
	char  *path;
	char  *uri;
	GFile *file;

	path = g_strconcat (name, file_extension, NULL);
	uri = _g_uri_append_path ("catalog:///", path);
	file = g_file_new_for_uri (uri);

	g_free (uri);
	g_free (path);

	return file;
}


char *
gth_catalog_get_relative_path (GFile *file)
{
	GFile *base;
	char  *path;

	base = gth_catalog_get_base ();
	path = g_file_get_relative_path (base, file);

	g_object_unref (base);

	return path;
}


GIcon *
gth_catalog_get_icon (GFile *file)
{
	char  *uri;
	GIcon *icon;

	uri = g_file_get_uri (file);
	if (g_str_has_suffix (uri, ".catalog"))
		icon = g_themed_icon_new ("file-catalog-symbolic");
	else
		icon = g_themed_icon_new ("file-library-symbolic");

	g_free (uri);

	return icon;
}


static char *
get_tag_value (const char *buffer,
	       const char *tag_start,
	       const char *tag_end)
{
	char *value;
	char *begin_tag;

	value = NULL;
	begin_tag = strstr (buffer, tag_start);
	if (begin_tag != NULL) {
		char        *end_tag;
		char        *xml;
		DomDocument *doc;

		end_tag = strstr (begin_tag, tag_end);
		xml = g_strndup (begin_tag, (end_tag - begin_tag) + strlen (tag_end));
		doc = dom_document_new ();
		if (dom_document_load (doc, xml, strlen (xml), NULL))
			value = g_strdup (dom_element_get_inner_text (DOM_ELEMENT (doc)->first_child));

		g_object_unref (doc);
		g_free (xml);
	}

	return value;
}


void
gth_catalog_update_standard_attributes (GFile     *file,
				        GFileInfo *info)
{
	char *display_name = NULL;
	char *edit_name = NULL;
	char *basename;

	basename = g_file_get_basename (file);
	if ((basename != NULL) && (strcmp (basename, "/") != 0)) {
		char        *name;
		GthDateTime *date_time;

		name = NULL;
		date_time = gth_datetime_new ();
		{
			GFile            *gio_file;
			GFileInputStream *istream;
			const int         buffer_size = 256;
			char              buffer[buffer_size];

			gio_file = gth_catalog_file_to_gio_file (file);
			istream = g_file_read (gio_file, NULL, NULL);
			if (istream != NULL) {
				gsize bytes_read;

				if (g_input_stream_read_all (G_INPUT_STREAM (istream),
							     buffer,
							     buffer_size - 1,
							     &bytes_read,
							     NULL,
							     NULL))
				{
					char *exif_date;

					buffer[bytes_read] = '\0';
					name = get_tag_value (buffer, "<name>", "</name>");
					exif_date = get_tag_value (buffer, "<date>", "</date>");
					if (exif_date != NULL)
						gth_datetime_from_exif_date (date_time, exif_date);

					g_free (exif_date);
				}
				g_object_unref (istream);
			}
			g_object_unref (gio_file);
		}

		update_standard_attributes (file, info, name, date_time);

		gth_datetime_free (date_time);
		g_free (name);
	}
	else {
		display_name = g_strdup (_("Catalogs"));
		edit_name = g_strdup (_("Catalogs"));
	}

	if (display_name != NULL)
		g_file_info_set_display_name (info, display_name);
	if (edit_name != NULL)
		g_file_info_set_edit_name (info, edit_name);

	g_free (edit_name);
	g_free (display_name);
	g_free (basename);
}


/* -- gth_catalog_load_from_file --*/


typedef struct {
	GFile         *file;
	ReadyCallback  ready_func;
	gpointer       user_data;
} LoadData;


static void
load__catalog_buffer_ready_cb (void     **buffer,
			       gsize      count,
			       GError    *error,
			       gpointer   user_data)
{
	LoadData   *load_data = user_data;
	GthCatalog *catalog;

	if (error == NULL) {
		catalog = gth_catalog_new_from_data (*buffer, count, &error);
		if (catalog == NULL)
			catalog = gth_catalog_new_for_file (load_data->file);
	}
	else
		catalog = NULL;
	load_data->ready_func (G_OBJECT (catalog), error, load_data->user_data);

	g_object_unref (load_data->file);
	g_free (load_data);
}


void
gth_catalog_load_from_file_async (GFile         *file,
				  GCancellable  *cancellable,
				  ReadyCallback  ready_func,
				  gpointer       user_data)
{
	LoadData *load_data;
	GFile    *gio_file;

	load_data = g_new0 (LoadData, 1);
	load_data->file = g_object_ref (file);
	load_data->ready_func = ready_func;
	load_data->user_data = user_data;

	gio_file = gth_catalog_file_to_gio_file (file);
	_g_file_load_async (gio_file,
			    G_PRIORITY_DEFAULT,
			    cancellable,
			    load__catalog_buffer_ready_cb,
			    load_data);

	g_object_unref (gio_file);
}


GFile *
gth_catalog_get_file_for_date (GthDateTime *date_time,
			       const char  *extension)
{
	char  *year;
	char  *uri;
	char  *display_name;
	GFile *catalog_file;

	year = gth_datetime_strftime (date_time, "%Y");
	uri = g_strconcat ("catalog:///", year, "/", NULL);
	display_name = gth_datetime_strftime (date_time, "%Y-%m-%d");
	catalog_file = _g_file_new_for_display_name (uri, display_name, extension);

	g_free (display_name);
	g_free (uri);
	g_free (year);

	return catalog_file;
}


GFile *
gth_catalog_get_file_for_tag (const char *tag,
			      const char *extension)
{
	char  *uri;
	GFile *catalog_file;

	uri = g_strconcat ("catalog:///", _("Tags"), "/", NULL);
	catalog_file = _g_file_new_for_display_name (uri, tag, extension);

	g_free (uri);

	return catalog_file;
}


GthCatalog *
gth_catalog_load_from_file (GFile *file)
{
	GthCatalog *catalog;
	GFile      *gio_file;
	void       *buffer;
	gsize       buffer_size;

	gio_file = gth_catalog_file_to_gio_file (file);
	if (! _g_file_load_in_buffer (gio_file, &buffer, &buffer_size, NULL, NULL))
		return NULL;

	catalog = gth_catalog_new_from_data (buffer, buffer_size, NULL);

	g_free (buffer);
	g_object_unref (gio_file);

	return catalog;
}


void
gth_catalog_save (GthCatalog *catalog)
{
	GFile  *file;
	GFile  *gio_file;
	GFile  *gio_parent;
	char   *data;
	gsize   size;
	GError *error = NULL;

	file = gth_catalog_get_file (catalog);
	gio_file = gth_catalog_file_to_gio_file (file);

	gio_parent = g_file_get_parent (gio_file);
	if (gio_parent != NULL)
		g_file_make_directory_with_parents (gio_parent, NULL, NULL);
	data = gth_catalog_to_data (catalog, &size);
	if (! _g_file_write (gio_file,
			     FALSE,
			     G_FILE_CREATE_NONE,
			     data,
			     size,
			     NULL,
			     &error))
	{
		g_warning ("%s", error->message);
		g_clear_error (&error);
	}
	else {
		GFile *parent_parent;
		GFile *parent;
		GList *list;

		parent = g_file_get_parent (file);
		parent_parent = g_file_get_parent (parent);
		if (parent_parent != NULL) {
			list = g_list_append (NULL, parent);
			gth_monitor_folder_changed (gth_main_get_default_monitor (),
						    parent_parent,
					            list,
						    GTH_MONITOR_EVENT_CREATED);
			g_list_free (list);
		}

		list = g_list_append (NULL, file);
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
				            parent,
				            list,
					    GTH_MONITOR_EVENT_CREATED);

		g_list_free (list);
		g_object_unref (parent);
	}

	g_free (data);
	_g_object_unref (gio_parent);
	g_object_unref (gio_file);
}


/* -- gth_catalog_list_async --  */


typedef struct {
	GthCatalog           *catalog;
	const char           *attributes;
	CatalogReadyCallback  list_ready_func;
	gpointer              user_data;
	GList                *current_file;
	GList                *files;
	GCancellable         *cancellable;
} ListData;


static void
gth_catalog_list_done (ListData *list_data,
		       GError   *error)
{
	if (list_data->list_ready_func != NULL) {
		list_data->files = g_list_reverse (list_data->files);
		list_data->list_ready_func (list_data->catalog, list_data->files, error, list_data->user_data);
	}

	_g_object_list_unref (list_data->files);
	_g_object_unref (list_data->cancellable);
	_g_object_unref (list_data->catalog);
	g_free (list_data);
}


static void
catalog_file_info_ready_cb (GObject      *source_object,
			    GAsyncResult *result,
			    gpointer      user_data)
{
	ListData   *list_data = user_data;
	GFile      *file;
	GFileInfo  *info;

	file = (GFile*) source_object;
	info = g_file_query_info_finish (file, result, NULL);
	if (info != NULL) {
		list_data->files = g_list_prepend (list_data->files, gth_file_data_new (file, info));
		g_object_unref (info);
	}

	list_data->current_file = list_data->current_file->next;
	if (list_data->current_file == NULL) {
		gth_catalog_list_done (list_data, NULL);
		return;
	}

	g_file_query_info_async ((GFile *) list_data->current_file->data,
				 list_data->attributes,
				 0,
				 G_PRIORITY_DEFAULT,
				 list_data->cancellable,
				 catalog_file_info_ready_cb,
				 list_data);
}


static void
list__catalog_buffer_ready_cb (void     **buffer,
			       gsize      count,
			       GError    *error,
			       gpointer   user_data)
{
	ListData *list_data = user_data;

	if ((error == NULL) && (*buffer != NULL)) {
		list_data->catalog = gth_catalog_new_from_data (*buffer,  count, &error);
		if (list_data->catalog == NULL) {
			gth_catalog_list_done (list_data, error);
			return;
		}

		list_data->current_file = list_data->catalog->priv->file_list;
		if (list_data->current_file == NULL) {
			gth_catalog_list_done (list_data, NULL);
			return;
		}

		g_file_query_info_async ((GFile *) list_data->current_file->data,
					 list_data->attributes,
					 0,
					 G_PRIORITY_DEFAULT,
					 list_data->cancellable,
					 catalog_file_info_ready_cb,
					 list_data);
	}
	else
		gth_catalog_list_done (list_data, error);
}


void
gth_catalog_list_async (GFile                *file,
			const char           *attributes,
			GCancellable         *cancellable,
			CatalogReadyCallback  ready_func,
			gpointer              user_data)
{
	ListData *list_data;

	list_data = g_new0 (ListData, 1);
	list_data->attributes = attributes;
	list_data->list_ready_func = ready_func;
	list_data->user_data = user_data;
	list_data->cancellable = _g_object_ref (cancellable);

	_g_file_load_async (file,
			    G_PRIORITY_DEFAULT,
			    list_data->cancellable,
			    list__catalog_buffer_ready_cb,
			    list_data);
}

