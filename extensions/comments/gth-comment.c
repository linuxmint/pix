/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 The Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-comment.h"


#define COMMENT_VERSION "3.0"


struct _GthCommentPrivate { /* All strings in utf8 format. */
	char       *caption;
	char       *note;
	char       *place;
	int         rating;
	GPtrArray  *categories;
	GDate      *date;
	GthTime    *time_of_day;
	gboolean    changed;
	gboolean    utf8;
};


static void gth_comment_gth_duplicable_interface_init (GthDuplicableInterface *iface);
static void gth_comment_dom_domizable_interface_init (DomDomizableInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GthComment,
			 gth_comment,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTH_TYPE_DUPLICABLE,
					 	gth_comment_gth_duplicable_interface_init)
		         G_IMPLEMENT_INTERFACE (DOM_TYPE_DOMIZABLE,
		        		 	gth_comment_dom_domizable_interface_init))


static void
gth_comment_free_data (GthComment *self)
{
	if (self->priv->place != NULL) {
		g_free (self->priv->place);
		self->priv->place = NULL;
	}

	if (self->priv->note != NULL) {
		g_free (self->priv->note);
		self->priv->note = NULL;
	}

	if (self->priv->caption != NULL) {
		g_free (self->priv->caption);
		self->priv->caption = NULL;
	}
}


static void
gth_comment_finalize (GObject *obj)
{
	GthComment *self = GTH_COMMENT (obj);

	gth_comment_free_data (self);
	gth_comment_clear_categories (self);
	g_ptr_array_free (self->priv->categories, TRUE);
	g_date_free (self->priv->date);
	gth_time_free (self->priv->time_of_day);

	G_OBJECT_CLASS (gth_comment_parent_class)->finalize (obj);
}


static void
gth_comment_class_init (GthCommentClass *klass)
{
	g_type_class_add_private (klass, sizeof (GthCommentPrivate));
	G_OBJECT_CLASS (klass)->finalize = gth_comment_finalize;
}


static void
gth_comment_init (GthComment *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GTH_TYPE_COMMENT, GthCommentPrivate);
	self->priv->caption = NULL;
	self->priv->note = NULL;
	self->priv->place = NULL;
	self->priv->rating = 0;
	self->priv->categories = g_ptr_array_new ();
	self->priv->date = g_date_new ();
	self->priv->time_of_day = gth_time_new ();
}


static GObject *
gth_comment_real_duplicate (GthDuplicable *base)
{
	return (GObject *) gth_comment_dup ((GthComment*) base);
}


static void
gth_comment_gth_duplicable_interface_init (GthDuplicableInterface *iface)
{
	iface->duplicate = gth_comment_real_duplicate;
}


static DomElement*
gth_comment_real_create_element (DomDomizable *base,
				 DomDocument  *doc)
{
	GthComment *self;
	DomElement *element;
	char       *value;
	GPtrArray  *categories;
	DomElement *categories_element;
	int         i;

	g_return_val_if_fail (DOM_IS_DOCUMENT (doc), NULL);

	self = GTH_COMMENT (base);
	element = dom_document_create_element (doc, "comment",
					       "version", COMMENT_VERSION,
					       NULL);

	dom_element_append_child (element, dom_document_create_element_with_text (doc, self->priv->caption, "caption", NULL));
	dom_element_append_child (element, dom_document_create_element_with_text (doc, self->priv->note, "note", NULL));
	dom_element_append_child (element, dom_document_create_element_with_text (doc, self->priv->place, "place", NULL));

	if (self->priv->rating > 0) {
		value = g_strdup_printf ("%d", self->priv->rating);
		dom_element_append_child (element, dom_document_create_element (doc, "rating", "value", value, NULL));
		g_free (value);
	}

	value = gth_comment_get_time_as_exif_format (self);
	if (value != NULL) {
		dom_element_append_child (element, dom_document_create_element (doc,  "time", "value", value, NULL));
		g_free (value);
	}

	categories = gth_comment_get_categories (self);
	categories_element = dom_document_create_element (doc, "categories", NULL);
	dom_element_append_child (element, categories_element);
	for (i = 0; i < categories->len; i++)
		dom_element_append_child (categories_element, dom_document_create_element (doc, "category", "value", g_ptr_array_index (categories, i), NULL));

	return element;
}


static void
gth_comment_real_load_from_element (DomDomizable *base,
				    DomElement   *element)
{
	GthComment *self;
	DomElement *node;

	g_return_if_fail (DOM_IS_ELEMENT (element));

	self = GTH_COMMENT (base);
	gth_comment_reset (self);

	if (g_strcmp0 (dom_element_get_attribute (element, "format"), "2.0") == 0) {
		for (node = element->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "Note") == 0)
				gth_comment_set_note (self, dom_element_get_inner_text (node));
			else if (g_strcmp0 (node->tag_name, "Place") == 0)
				gth_comment_set_place (self, dom_element_get_inner_text (node));
			else if (g_strcmp0 (node->tag_name, "Time") == 0)
				gth_comment_set_time_from_time_t (self, atol (dom_element_get_inner_text (node)));
			else if (g_strcmp0 (node->tag_name, "Keywords") == 0) {
				const char  *text;

				text = dom_element_get_inner_text (node);
				if (text != NULL) {
					char **categories;
					int    i;

					categories = g_strsplit (text, ",", -1);
					for (i = 0; categories[i] != NULL; i++)
						gth_comment_add_category (self, categories[i]);
					g_strfreev (categories);
				}
			}
		}
	}
	else if (g_strcmp0 (dom_element_get_attribute (element, "version"), "3.0") == 0) {
		for (node = element->first_child; node; node = node->next_sibling) {
			if (g_strcmp0 (node->tag_name, "caption") == 0)
				gth_comment_set_caption (self, dom_element_get_inner_text (node));
			else if (g_strcmp0 (node->tag_name, "note") == 0)
				gth_comment_set_note (self, dom_element_get_inner_text (node));
			else if (g_strcmp0 (node->tag_name, "place") == 0)
				gth_comment_set_place (self, dom_element_get_inner_text (node));
			else if (g_strcmp0 (node->tag_name, "time") == 0)
				gth_comment_set_time_from_exif_format (self, dom_element_get_attribute (node, "value"));
			else if (g_strcmp0 (node->tag_name, "rating") == 0) {
				int v;

				sscanf (dom_element_get_attribute (node, "value"), "%d", &v);
				gth_comment_set_rating (self, v);
			}
			else if (g_strcmp0 (node->tag_name, "categories") == 0) {
				DomElement *child;

				for (child = node->first_child; child != NULL; child = child->next_sibling)
					if (strcmp (child->tag_name, "category") == 0)
						gth_comment_add_category (self, dom_element_get_attribute (child, "value"));
			}
		}
	}
}


static void
gth_comment_dom_domizable_interface_init (DomDomizableInterface *iface)
{
	iface->create_element = gth_comment_real_create_element;
	iface->load_from_element = gth_comment_real_load_from_element;
}


GthComment *
gth_comment_new (void)
{
	return g_object_new (GTH_TYPE_COMMENT, NULL);
}


GFile *
gth_comment_get_comment_file (GFile *file)
{
	GFile *parent;
	char  *basename;
	char  *comment_basename;
	GFile *comment_file;

	parent = g_file_get_parent (file);
	if (parent == NULL)
		return NULL;

	basename = g_file_get_basename (file);
	comment_basename = g_strconcat (basename, ".xml", NULL);
	comment_file = _g_file_get_child (parent, ".comments", comment_basename, NULL);

	g_free (comment_basename);
	g_free (basename);
	g_object_unref (parent);

	return comment_file;
}


GthComment *
gth_comment_new_for_file (GFile         *file,
			  GCancellable  *cancellable,
			  GError       **error)
{
	GFile       *comment_file;
	GthComment  *comment;
	void        *zipped_buffer;
	gsize        zipped_size;
	void        *buffer;
	gsize        size;
	DomDocument *doc;

	comment_file = gth_comment_get_comment_file (file);
	if (comment_file == NULL)
		return NULL;

	if (! _g_file_load_in_buffer (comment_file, &zipped_buffer, &zipped_size, cancellable, error)) {
		g_object_unref (comment_file);
		return NULL;
	}
	g_object_unref (comment_file);

	if ((zipped_buffer != NULL) && (((char *) zipped_buffer)[0] != '<')) {
		if (! zlib_decompress_buffer (zipped_buffer, zipped_size, &buffer, &size))
			return NULL;
	}
	else {
		buffer = zipped_buffer;
		size = zipped_size;

		zipped_buffer = NULL;
	}

	comment = gth_comment_new ();
	doc = dom_document_new ();
	if (dom_document_load (doc, buffer, size, error)) {
		dom_domizable_load_from_element (DOM_DOMIZABLE (comment), DOM_ELEMENT (doc)->first_child);
	}
	else {
		buffer = NULL;
		g_object_unref (comment);
		comment = NULL;
	}

	g_object_unref (doc);
	g_free (buffer);
	g_free (zipped_buffer);

	return comment;
}


char *
gth_comment_to_data (GthComment *comment,
		     gsize      *length)
{
	DomDocument *doc;
	char        *data;

	doc = dom_document_new ();
	dom_element_append_child (DOM_ELEMENT (doc), dom_domizable_create_element (DOM_DOMIZABLE (comment), doc));
	data = dom_document_dump (doc, length);

	g_object_unref (doc);

	return data;
}


GthComment *
gth_comment_dup (GthComment *self)
{
	GthComment *comment;
	char       *time;
	int         i;

	if (self == NULL)
		return NULL;

	comment = gth_comment_new ();
	gth_comment_set_caption (comment, gth_comment_get_caption (self));
	gth_comment_set_note (comment, gth_comment_get_note (self));
	gth_comment_set_place (comment, gth_comment_get_place (self));
	gth_comment_set_rating (comment, gth_comment_get_rating (self));
	time = gth_comment_get_time_as_exif_format (self);
	gth_comment_set_time_from_exif_format (comment, time);
	for (i = 0; i < self->priv->categories->len; i++)
		gth_comment_add_category (comment, g_ptr_array_index (self->priv->categories, i));

	g_free (time);

	return comment;
}


void
gth_comment_reset (GthComment *self)
{
	gth_comment_free_data (self);
	gth_comment_clear_categories (self);
	gth_comment_reset_time (self);
}


void
gth_comment_set_caption (GthComment  *comment,
			 const char  *value)
{
	g_free (comment->priv->caption);
	comment->priv->caption = NULL;

	if ((value != NULL) && (strcmp (value, "") != 0))
		comment->priv->caption = g_strdup (value);
}


void
gth_comment_set_note (GthComment *comment,
		      const char *value)
{
	g_free (comment->priv->note);
	comment->priv->note = NULL;

	if ((value != NULL) && (strcmp (value, "") != 0))
		comment->priv->note = g_strdup (value);
}


void
gth_comment_set_place (GthComment *comment,
		       const char *value)
{
	g_free (comment->priv->place);
	comment->priv->place = NULL;

	if ((value != NULL) && (strcmp (value, "") != 0))
		comment->priv->place = g_strdup (value);
}


void
gth_comment_set_rating (GthComment *comment,
		        int         value)
{
	comment->priv->rating = value;
}


void
gth_comment_clear_categories (GthComment *self)
{
	g_ptr_array_foreach (self->priv->categories, (GFunc) g_free, NULL);
	g_ptr_array_free (self->priv->categories, TRUE);
	self->priv->categories = g_ptr_array_new ();
}


void
gth_comment_add_category (GthComment *comment,
			  const char *value)
{
	g_return_if_fail (value != NULL);

	g_ptr_array_add (comment->priv->categories, g_strdup (value));
}


void
gth_comment_reset_time (GthComment *self)
{
	g_date_clear (self->priv->date, 1);
	gth_time_clear (self->priv->time_of_day);
}


void
gth_comment_set_time_from_exif_format (GthComment *comment,
				       const char *value)
{
	unsigned int y, m, d, hh, mm, ss;

	gth_comment_reset_time (comment);

	if ((value == NULL) || (*value == '\0'))
		return;

	if (sscanf (value, "%u:%u:%u %u:%u:%u", &y, &m, &d, &hh, &mm, &ss) != 6) {
		g_warning ("invalid time format: %s", value);
		return;
	}

	if (g_date_valid_dmy (d, m, y)) {
		g_date_set_dmy (comment->priv->date, d, m, y);
		gth_time_set_hms (comment->priv->time_of_day, hh, mm, ss, 0);
	}
}


void
gth_comment_set_time_from_time_t (GthComment *comment,
				  time_t      value)
{
	struct tm *tm;

	if (value == 0)
		return;

	tm = localtime (&value);
	g_date_set_dmy (comment->priv->date, tm->tm_mday, tm->tm_mon + 1, 1900 + tm->tm_year);
	gth_time_set_hms (comment->priv->time_of_day, tm->tm_hour, tm->tm_min, tm->tm_sec, 0);
}


const char *
gth_comment_get_caption (GthComment *comment)
{
	return comment->priv->caption;
}


const char *
gth_comment_get_note (GthComment *comment)
{
	return comment->priv->note;
}


const char *
gth_comment_get_place (GthComment *comment)
{
	return comment->priv->place;
}


int
gth_comment_get_rating (GthComment *comment)
{
	return comment->priv->rating;
}


GPtrArray *
gth_comment_get_categories (GthComment *comment)
{
	return comment->priv->categories;
}


GDate *
gth_comment_get_date (GthComment *comment)
{
	return comment->priv->date;
}


GthTime *
gth_comment_get_time_of_day (GthComment *comment)
{
	return comment->priv->time_of_day;
}


char *
gth_comment_get_time_as_exif_format (GthComment *comment)
{
	char *s;

	if (! g_date_valid (comment->priv->date))
		return NULL;

	s = g_strdup_printf ("%04u:%02u:%02u %02u:%02u:%02u",
			     g_date_get_year (comment->priv->date),
			     g_date_get_month (comment->priv->date),
			     g_date_get_day (comment->priv->date),
			     comment->priv->time_of_day->hour,
			     comment->priv->time_of_day->min,
			     comment->priv->time_of_day->sec);

	return s;
}


void
gth_comment_update_general_attributes (GthFileData *file_data)
{
	const char *value;

	value = g_file_info_get_attribute_string (file_data->info, "comment::note");
	if (value != NULL)
		set_attribute_from_string (file_data->info,
					   "general::description",
					   value,
					   NULL);

	value = g_file_info_get_attribute_string (file_data->info, "comment::caption");
	if (value != NULL)
		set_attribute_from_string (file_data->info,
					   "general::title",
					   value,
					   NULL);

	value = g_file_info_get_attribute_string (file_data->info, "comment::place");
	if (value != NULL)
		set_attribute_from_string (file_data->info,
					   "general::location",
					   value,
					   NULL);

	if (g_file_info_has_attribute (file_data->info, "comment::rating")) {
		char *v;

		v = g_strdup_printf ("%d", g_file_info_get_attribute_int32 (file_data->info, "comment::rating"));
		set_attribute_from_string (file_data->info, "general::rating", v, NULL);

		g_free (v);
	}

	if (g_file_info_has_attribute (file_data->info, "comment::categories"))
		g_file_info_set_attribute_object (file_data->info,
						  "general::tags",
						  g_file_info_get_attribute_object (file_data->info, "comment::categories"));

	if (g_file_info_has_attribute (file_data->info, "comment::time"))
		g_file_info_set_attribute_object (file_data->info,
						  "general::datetime",
						  g_file_info_get_attribute_object (file_data->info, "comment::time"));
}


void
gth_comment_update_from_general_attributes (GthFileData *file_data)
{
	gboolean       write_comment;
	GthMetadata   *metadata;
	GthStringList *comment_categories;
	GList         *scan;
	const char    *text;
	GthComment    *comment;
	GthStringList *categories;

	write_comment = FALSE;

	comment = gth_comment_new ();
	gth_comment_set_note (comment, g_file_info_get_attribute_string (file_data->info, "comment::note"));
	gth_comment_set_caption (comment, g_file_info_get_attribute_string (file_data->info, "comment::caption"));
	gth_comment_set_place (comment, g_file_info_get_attribute_string (file_data->info, "comment::place"));

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
	if (metadata != NULL)
		gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
	comment_categories = gth_metadata_get_string_list (metadata);
	if (comment_categories != NULL)
		for (scan = gth_string_list_get_list (comment_categories); scan; scan = scan->next)
			gth_comment_add_category (comment, (char *) scan->data);

	gth_comment_set_rating (comment, g_file_info_get_attribute_int32 (file_data->info, "comment::rating"));

	/* sync embedded data and .comment data if required */

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::description");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::note");
		if (! dom_str_equal (gth_metadata_get_formatted (metadata), text)) {
			gth_comment_set_note (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::title");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::caption");
		if (! dom_str_equal (gth_metadata_get_formatted (metadata), text)) {
			gth_comment_set_caption (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::location");
	if (metadata != NULL) {
		text = g_file_info_get_attribute_string (file_data->info, "comment::place");
		if (! dom_str_equal (gth_metadata_get_formatted (metadata), text)) {
			gth_comment_set_place (comment, gth_metadata_get_formatted (metadata));
			write_comment = TRUE;
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::datetime");
	if (metadata != NULL) {
		GthMetadata *comment_time;

		text = gth_metadata_get_raw (metadata);
		comment_time = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::time");
		if (comment_time != NULL) {
			if (! dom_str_equal (gth_metadata_get_raw (comment_time), text)) {
				gth_comment_set_time_from_exif_format (comment, gth_metadata_get_raw (metadata));
				write_comment = TRUE;
			}
		}
	}

	metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "general::tags");
	categories = gth_metadata_get_string_list (metadata);
	if (categories != NULL) {
		metadata = (GthMetadata *) g_file_info_get_attribute_object (file_data->info, "comment::categories");
		comment_categories = gth_metadata_get_string_list (metadata);
		if (! gth_string_list_equal (categories, comment_categories)) {
			GList *scan;

			gth_comment_clear_categories (comment);
			for (scan = gth_string_list_get_list (categories); scan; scan = scan->next)
				gth_comment_add_category (comment, scan->data);
			write_comment = TRUE;
		}
	}

	if (write_comment) {
		GFile *comment_file;
		GFile *comment_directory;
		char  *buffer;
		gsize  size;

		comment_file = gth_comment_get_comment_file (file_data->file);
		comment_directory = g_file_get_parent (comment_file);
		if (! g_file_query_exists (comment_directory, NULL))
			g_file_make_directory (comment_directory, NULL, NULL);

		buffer = gth_comment_to_data (comment, &size);
		if (_g_file_write (comment_file,
				   FALSE,
				   G_FILE_CREATE_NONE,
				   buffer,
				   size,
				   NULL,
				   NULL))
		{
			GFile *parent;
			GList *list;

			parent = g_file_get_parent (file_data->file);
			list = g_list_prepend (NULL, file_data->file);
			gth_monitor_folder_changed (gth_main_get_default_monitor (),
						    parent,
						    list,
						    GTH_MONITOR_EVENT_CHANGED);

			g_list_free (list);
			g_object_unref (parent);
		}

		g_free (buffer);
		g_object_unref (comment_directory);
		g_object_unref (comment_file);
	}

	g_object_unref (comment);
}
