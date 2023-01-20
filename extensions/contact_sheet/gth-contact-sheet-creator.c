/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/image_rotation/rotation-utils.h>
#include "gth-contact-sheet-creator.h"
#include "preferences.h"

#define DEFAULT_THUMB_SIZE 128
#define DEFAULT_FONT "Sans 12"


typedef struct {
	GthFileData     *file_data;
	cairo_surface_t *thumbnail;
	int              original_width;
	int              original_height;
} ItemData;


static ItemData *
item_data_new (GthFileData *file_data)
{
	ItemData *item_data;

	item_data = g_new0 (ItemData, 1);
	item_data->file_data = g_object_ref (file_data);
	item_data->thumbnail = NULL;
	item_data->original_width = 0;
	item_data->original_height = 0;

	return item_data;
}


static void
item_data_free (ItemData *item_data)
{
	cairo_surface_destroy (item_data->thumbnail);
	_g_object_unref (item_data->file_data);
	g_free (item_data);
}


struct _GthContactSheetCreatorPrivate {
	GthBrowser           *browser;
	GList                *gfile_list;             /* GFile list */

	/* options */

	char                 *header;
	char                 *footer;
	GFile                *destination;
	GFile                *destination_file;
	char                 *template;
	char                 *mime_type;
	char                 *file_extension;
	gboolean              write_image_map;
	GthContactSheetTheme *theme;
	int                   images_per_index;
	gboolean              single_index;
	int                   columns_per_page;
	int                   rows_per_page;
	GthFileDataSort      *sort_type;
	gboolean              sort_inverse;
	gboolean              same_size;
	gboolean              squared_thumbnails;
	int                   thumb_width;
	int                   thumb_height;
	char                 *thumbnail_caption;
	char                 *location_name;

	/* private data */

	cairo_t              *cr;
	PangoContext         *pango_context;
	PangoLayout          *pango_layout;

	GthImageLoader       *image_loader;
	GList                *files;                /* ItemData list */
	GList                *current_file;         /* Next file to be loaded. */
	gint                  n_files;              /* Used for the progress signal. */
	gint                  n_loaded_files;
	GList                *created_files;
	GFile                *imagemap_file;
	GDataOutputStream    *imagemap_stream;

	int                   page_width;
	int                   page_height;
	int                  *pages_height;
	int                   n_pages;
	char                **thumbnail_caption_v;
	char                **template_v;
	GDateTime            *timestamp;
};


G_DEFINE_TYPE_WITH_CODE (GthContactSheetCreator,
			 gth_contact_sheet_creator,
			 GTH_TYPE_TASK,
			 G_ADD_PRIVATE (GthContactSheetCreator))


static int
get_text_height (GthContactSheetCreator *self,
		 const char             *text,
		 const char             *font_name,
		 int                     width)
{
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;

	if (text == NULL)
		return 0;

	if (font_name != NULL)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (self->priv->pango_layout, font_desc);
	pango_layout_set_width (self->priv->pango_layout, width * PANGO_SCALE);
	pango_layout_set_wrap (self->priv->pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_text (self->priv->pango_layout, text, -1);

	pango_layout_get_pixel_extents (self->priv->pango_layout, NULL, &bounds);

	if (font_desc != NULL)
		pango_font_description_free (font_desc);

	return bounds.height;
}


static int
get_header_height (GthContactSheetCreator *self,
		   int                     with_spacing)
{
	int h;

	if ((self->priv->header == NULL) || (strcmp (self->priv->header, "") == 0))
		return 0;

	h = get_text_height (self,
			     self->priv->header,
			     self->priv->theme->header_font_name,
			     self->priv->page_width);
	if (with_spacing)
		h += (self->priv->theme->row_spacing * 2);

	return h;
}


static int
get_footer_height (GthContactSheetCreator *self,
		   int                     with_spacing)
{
	int h;

	if ((self->priv->footer == NULL) || (strcmp (self->priv->footer, "") == 0))
		return 0;

	h = get_text_height (self,
			     self->priv->footer,
			     self->priv->theme->footer_font_name,
			     self->priv->page_width);
	if (with_spacing)
		h += (self->priv->theme->row_spacing * 2);

	return h;
}


static int
get_max_text_height (GthContactSheetCreator *self,
		     GList                  *first_item,
		     GList                  *last_item)
{
	int    max_height = 0;
	GList *scan;

	for (scan = first_item; scan != last_item; scan = scan->next) {
		ItemData *item_data = scan->data;
		int       text_height;
		int       i;

		text_height = 0;
		for (i = 0; self->priv->thumbnail_caption_v[i] != NULL; i++) {
			char *value;

			value = gth_file_data_get_attribute_as_string (item_data->file_data, self->priv->thumbnail_caption_v[i]);
			if (value != NULL) {
				text_height += get_text_height (self, value, self->priv->theme->caption_font_name, self->priv->thumb_width);
				text_height += self->priv->theme->caption_spacing;
			}

			g_free (value);
		}

		max_height = MAX (max_height, text_height);
	}

	return max_height;
}


static int
get_page_height (GthContactSheetCreator *self,
		 int                     page_n)
{
	return self->priv->same_size ? self->priv->page_height : self->priv->pages_height[page_n - 1];
}


typedef struct {
	GthContactSheetCreator *creator;
	int                     page;
} TemplateData;


static gboolean
filename_template_eval_cb (TemplateFlags   flags,
			   gunichar        parent_code,
			   gunichar        code,
			   char          **args,
			   GString        *result,
			   gpointer        user_data)
{
	TemplateData *template_data = user_data;
	char         *text = NULL;

	if (parent_code == 'D') {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	switch (code) {
	case '#':
		text = _g_template_replace_enumerator (args[0], template_data->page);
		break;

	case 'D':
		text = g_date_time_format (template_data->creator->priv->timestamp,
					   (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;

	default:
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	return FALSE;
}


static void
begin_page (GthContactSheetCreator *self,
	    int                     page_n)
{
	TemplateData     template_data;
	char            *name;
	char            *display_name;
	int              width;
	int              height;
	cairo_surface_t *surface;

	template_data.creator = self;
	template_data.page = page_n - 1;
	name = _g_template_eval (self->priv->template,
				 0,
				 filename_template_eval_cb,
				 &template_data);
	display_name = g_strdup_printf ("%s.%s", name, self->priv->file_extension);
	_g_object_unref (self->priv->destination_file);
	self->priv->destination_file = g_file_get_child_for_display_name (self->priv->destination, display_name, NULL);

	gth_task_progress (GTH_TASK (self),
			   _("Creating images"),
			   display_name,
			   FALSE,
			   (double) page_n / self->priv->n_pages);
	g_free (display_name);

	width = self->priv->page_width;
	height = get_page_height (self, page_n);

	if (self->priv->cr != NULL)
		cairo_destroy (self->priv->cr);
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	self->priv->cr = cairo_create (surface);
	cairo_surface_destroy (surface);

	gth_contact_sheet_theme_paint_background (self->priv->theme, self->priv->cr, width, height);

	/* image map file. */

	if (self->priv->write_image_map) {
		char              *display_name;
		GError            *error = NULL;
		GFileOutputStream *io_stream;
		char              *uri;
		char              *line;

		_g_object_unref (self->priv->imagemap_file);
		display_name = g_strdup_printf ("%s.html", name);
		self->priv->imagemap_file = g_file_get_child_for_display_name (self->priv->destination, display_name, &error);
		g_free (display_name);

		if (error != NULL) {
			g_warning ("%s\n", error->message);
			g_clear_error (&error);
			return;
		}

		io_stream = g_file_replace (self->priv->imagemap_file,
					    NULL,
					    FALSE,
					    G_FILE_CREATE_NONE,
					    gth_task_get_cancellable (GTH_TASK (self)),
					    &error);
		if (io_stream == NULL) {
			g_warning ("%s\n", error->message);
			g_clear_error (&error);
			return;
		}

		_g_object_unref (self->priv->imagemap_stream);
		self->priv->imagemap_stream = g_data_output_stream_new (G_OUTPUT_STREAM (io_stream));

		line = g_strdup_printf ("\
<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n\
  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\
<head>\n\
  <title>%s</title>\n\
  <style type=\"text/css\">\n\
    html { margin: 0px; border: 0px; padding: 0px; }\n\
    body { margin: 0px; }\n\
    img  { border: 0px; }\n\
  </style>\n\
</head>\n\
<body>\n\
  <div>\n\
",
					self->priv->header);
		g_data_output_stream_put_string (self->priv->imagemap_stream,
						 line,
						 gth_task_get_cancellable (GTH_TASK (self)),
						 &error);
		g_free (line);

		uri = g_file_get_uri (self->priv->destination_file);
		line = g_strdup_printf ("    <img src=\"%s\" width=\"%d\" height=\"%d\" usemap=\"#map\" alt=\"%s\" />\n",
					uri,
					width,
					height,
					uri);
		g_data_output_stream_put_string (self->priv->imagemap_stream,
						 line,
						 gth_task_get_cancellable (GTH_TASK (self)),
						 &error);
		g_free (line);

		g_data_output_stream_put_string (self->priv->imagemap_stream,
						 "    <map name=\"map\" id=\"map\">\n",
						 gth_task_get_cancellable (GTH_TASK (self)),
						 &error);

		g_free (uri);
	}

	g_free (name);
}


static gboolean
end_page (GthContactSheetCreator  *self,
	  int                      page_n,
	  GError                 **error)
{
	GthImage *image;
	char     *buffer;
	gsize     size;

	image = gth_image_new ();
	gth_image_set_cairo_surface (image, cairo_get_target (self->priv->cr));
	if (! gth_image_save_to_buffer (image,
					self->priv->mime_type,
					NULL,
					&buffer,
					&size,
					gth_task_get_cancellable (GTH_TASK (self)),
					error))
	{
		g_object_unref (image);
		return FALSE;
	}

	if (! _g_file_write (self->priv->destination_file,
			     FALSE,
			     G_FILE_CREATE_REPLACE_DESTINATION,
			     buffer,
			     size,
			     gth_task_get_cancellable (GTH_TASK (self)),
			     error))
	{
		g_object_unref (image);
		return FALSE;
	}

	self->priv->created_files = g_list_prepend (self->priv->created_files,
						    g_object_ref (self->priv->destination_file));

	g_object_unref (image);

	/* image map file. */

	if (self->priv->imagemap_stream != NULL) {
		if (! g_data_output_stream_put_string (self->priv->imagemap_stream,
						       "    </map>\n",
						       gth_task_get_cancellable (GTH_TASK (self)),
						       error))
		{
			return FALSE;
		}

		if (! g_data_output_stream_put_string (self->priv->imagemap_stream,
						       "\
  </div>\n\
</body>\n\
</html>\n\
",
						       gth_task_get_cancellable (GTH_TASK (self)),
						       error))
		{
			return FALSE;
		}

		if (! g_output_stream_close (G_OUTPUT_STREAM (self->priv->imagemap_stream),
					     gth_task_get_cancellable (GTH_TASK (self)),
					     error))
		{
			return FALSE;
		}
		self->priv->created_files = g_list_prepend (self->priv->created_files, g_object_ref (self->priv->imagemap_file));
	}

	return TRUE;
}


/* -- get_text -- */


static gboolean
text_template_eval_cb (TemplateFlags   flags,
		       gunichar        parent_code,
		       gunichar        code,
		       char          **args,
		       GString        *result,
		       gpointer        user_data)
{
	TemplateData *template_data = user_data;
	char         *text = NULL;

	if (parent_code == 'D') {
		/* strftime code, return the code itself. */
		_g_string_append_template_code (result, code, args);
		return FALSE;
	}

	switch (code) {
	case 'p':
		text = g_strdup_printf ("%d", template_data->page);
		break;

	case 'n':
		text = g_strdup_printf ("%d", template_data->creator->priv->n_pages);
		break;

	case 'D':
		text = g_date_time_format (template_data->creator->priv->timestamp,
					   (args[0] != NULL) ? args[0] : DEFAULT_STRFTIME_FORMAT);
		break;

	case 'L':
		if (template_data->creator->priv->location_name != NULL)
			g_string_append (result, template_data->creator->priv->location_name);
		break;

	default:
		break;
	}

	if (text != NULL) {
		g_string_append (result, text);
		g_free (text);
	}

	return FALSE;
}


static char *
get_text (GthContactSheetCreator *self,
	  const char             *text,
	  int                     page_n)
{
	TemplateData template_data;

	template_data.creator = self;
	template_data.page = page_n;
	return _g_template_eval (text,
				 0,
				 text_template_eval_cb,
				 &template_data);
}


static void
paint_text (GthContactSheetCreator *self,
	    const char             *font_name,
	    GdkRGBA                *color,
	    int                     x,
	    int                     y,
	    int                     width,
	    const char             *text,
	    int                    *height)
{
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;

	if (font_name != NULL)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);
	pango_layout_set_font_description (self->priv->pango_layout, font_desc);
	pango_layout_set_width (self->priv->pango_layout, width * PANGO_SCALE);
	pango_layout_set_wrap (self->priv->pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_text (self->priv->pango_layout, text, -1);
	pango_layout_get_pixel_extents (self->priv->pango_layout, NULL, &bounds);

	x += self->priv->theme->frame_border;

	cairo_save (self->priv->cr);
	gdk_cairo_set_source_rgba (self->priv->cr, color);
	pango_cairo_update_layout (self->priv->cr, self->priv->pango_layout);
	cairo_move_to (self->priv->cr, x, y);
	pango_cairo_show_layout (self->priv->cr, self->priv->pango_layout);
	cairo_restore (self->priv->cr);

#if 0
	cairo_set_source_rgb (self->priv->cr, 1.0, 0.0, 0.0);
	cairo_rectangle (self->priv->cr, x, y, width, bounds.height);
	cairo_stroke (self->priv->cr);
#endif

	if (font_desc != NULL)
		pango_font_description_free (font_desc);

	if (height != NULL)
		*height = bounds.height;
}


static void
paint_footer (GthContactSheetCreator *self,
	      int                     page_n)
{
	char *text;

	if (self->priv->footer == NULL)
		return;

	text = get_text (self, self->priv->footer, page_n);
	paint_text (self,
		    self->priv->theme->footer_font_name,
		    &self->priv->theme->footer_color,
		    0,
		    get_page_height (self, page_n) - get_footer_height (self, FALSE) - self->priv->theme->row_spacing / 2,
		    self->priv->page_width,
		    text,
		    NULL);

	g_free (text);
}


static void
paint_frame (GthContactSheetCreator *self,
	     cairo_rectangle_int_t  *frame_rect,
	     cairo_rectangle_int_t  *image_rect,
	     GthFileData            *file_data)
{
	gth_contact_sheet_theme_paint_frame (self->priv->theme, self->priv->cr, frame_rect, image_rect);

	if (self->priv->imagemap_stream != NULL) {
		char   *file;
		char   *destination;
		char   *relative_path;
		char   *alt_attribute;
		char   *line;
		GError *error = NULL;

		file = g_file_get_uri (file_data->file);
		destination = g_file_get_uri (self->priv->destination);
		relative_path = _g_uri_get_relative_path (file, destination);
		alt_attribute = _g_utf8_escape_xml (relative_path);

		line = g_strdup_printf ("      <area shape=\"rect\" coords=\"%d,%d,%d,%d\" href=\"%s\" alt=\"%s\" />\n",
					frame_rect->x,
					frame_rect->y,
					frame_rect->x + frame_rect->width,
					frame_rect->y + frame_rect->height,
					relative_path,
					alt_attribute);
		g_data_output_stream_put_string (self->priv->imagemap_stream,
						 line,
						 gth_task_get_cancellable (GTH_TASK (self)),
						 &error);

		g_free (line);
		g_free (alt_attribute);
		g_free (relative_path);
		g_free (destination);
		g_free (file);
	}
}


static void
paint_image (GthContactSheetCreator *self,
	     cairo_rectangle_int_t  *image_rect,
	     cairo_surface_t        *image)
{
	cairo_save (self->priv->cr);
	cairo_set_source_surface (self->priv->cr, image, image_rect->x, image_rect->y);
  	cairo_rectangle (self->priv->cr, image_rect->x, image_rect->y, image_rect->width, image_rect->height);
  	cairo_fill (self->priv->cr);
  	cairo_restore (self->priv->cr);
}


static void
export (GthContactSheetCreator *self)
{
	int        columns;
	gboolean   first_row;
	int        page_n = 0;
	int        header_height;
	int        footer_height;
	int        x, y;
	GList     *scan;
	GError    *error = NULL;

	if (self->priv->timestamp != NULL)
		g_date_time_unref (self->priv->timestamp);
	self->priv->timestamp = g_date_time_new_now_local ();

	columns = ((self->priv->page_width - self->priv->theme->col_spacing) / (self->priv->thumb_width + (self->priv->theme->frame_hpadding * 2) + self->priv->theme->col_spacing));
	first_row = TRUE;
	begin_page (self, ++page_n);
	header_height = get_header_height (self, TRUE);
	footer_height = get_footer_height (self, TRUE);
	y = self->priv->theme->col_spacing;
	scan = self->priv->files;
	do {
		GList *first_item, *last_item;
		int    i;
		int    row_height;
		GList *scan_row;

		/* get items to paint. */

		first_item = scan;
		last_item = NULL;
		for (i = 0; i < columns; i++) {
			if (scan == NULL) {
				columns = i;
				break;
			}
			last_item = scan = scan->next;
		}

		if (columns == 0) {
			paint_footer (self, page_n);
			end_page (self, page_n, &error);
			goto export_end;
		}

		/* check whether the row fit the current page. */

		row_height = (self->priv->thumb_height
			      + (self->priv->theme->frame_vpadding * 2)
			      + get_max_text_height (self, first_item, last_item)
			      + self->priv->theme->row_spacing);

		while (y + row_height > (self->priv->page_height
					 - (first_row ? header_height : 0)
					 - footer_height))
		{
			if (first_row) {
				/* this row has an height greater than the
				 * page height, close and exit. */
				goto export_end;
			}

			/* the row does not fit this page, create a new
			 * page. */

			if (page_n > 0) {
				paint_footer (self, page_n);
				if (! end_page (self, page_n, &error))
					goto export_end;
			}

			first_row = TRUE;
			begin_page (self, ++page_n);
			header_height = get_header_height (self, TRUE);
			footer_height = get_footer_height (self, TRUE);
			y = self->priv->theme->row_spacing;
		}

		/* paint the header. */

		if (first_row && (g_strcmp0 (self->priv->header, "") != 0)) {
			char *text;

			text = get_text (self, self->priv->header, page_n);
			paint_text (self,
				    self->priv->theme->header_font_name,
				    &self->priv->theme->header_color,
				    0,
				    y,
				    self->priv->page_width,
				    text,
				    NULL);
			g_free (text);

			y += header_height;
		}

		/* paint a row. */

		x = self->priv->theme->col_spacing;
		for (scan_row = first_item; scan_row != last_item; scan_row = scan_row->next) {
			ItemData              *row_item = scan_row->data;
			int                    frame_width;
			int                    frame_height;
			cairo_rectangle_int_t  frame_rect;
			int                    text_y;
			int                    i;

			frame_width = self->priv->thumb_width + (self->priv->theme->frame_hpadding * 2);
			frame_height = self->priv->thumb_height + (self->priv->theme->frame_vpadding * 2);
			frame_rect.x = x;
			frame_rect.y = y;
			frame_rect.width = frame_width;
			frame_rect.height = frame_height;

			/* paint the thumbnail */

			if (row_item->thumbnail != NULL) {
				int                   thumbnail_width;
				int                   thumbnail_height;
				cairo_rectangle_int_t image_rect;

				thumbnail_width = cairo_image_surface_get_width (row_item->thumbnail);
				thumbnail_height = cairo_image_surface_get_height (row_item->thumbnail);

				image_rect.x = x + (frame_width - thumbnail_width) / 2;
				image_rect.y = y + (frame_height - thumbnail_height) / 2;
				image_rect.width = thumbnail_width;
				image_rect.height = thumbnail_height;

				paint_frame (self, &frame_rect, &image_rect, row_item->file_data);
				paint_image (self, &image_rect, row_item->thumbnail);
			}

			/* paint the caption */

			text_y = y + frame_height + self->priv->theme->caption_spacing;

			for (i = 0; self->priv->thumbnail_caption_v[i] != NULL; i++) {
				char *text;
				int   h;

				text = gth_file_data_get_attribute_as_string (row_item->file_data, self->priv->thumbnail_caption_v[i]);
				if (text == NULL)
					continue;

				paint_text (self,
					    self->priv->theme->caption_font_name,
					    &self->priv->theme->caption_color,
					    x + self->priv->theme->frame_hpadding,
					    text_y,
					    self->priv->thumb_width,
					    text,
					    &h);
				text_y += h + self->priv->theme->caption_spacing;

				g_free (text);
			}

			x += self->priv->thumb_width + (self->priv->theme->frame_hpadding * 2) + self->priv->theme->col_spacing;
		}

		y += row_height;
		first_row = FALSE;
	}
	while (TRUE);

 export_end:

	if (self->priv->created_files != NULL) {
		gth_monitor_folder_changed (gth_main_get_default_monitor (),
					    self->priv->destination,
					    self->priv->created_files,
					    GTH_MONITOR_EVENT_CREATED);
		if (error == NULL) {
			GtkWidget *new_window;

			new_window = gth_browser_new ((GFile *) self->priv->created_files->data,
						      NULL);
			gtk_window_present (GTK_WINDOW (new_window));
		}

		_g_object_list_unref (self->priv->created_files);
		self->priv->created_files = NULL;
	}

	gth_task_completed (GTH_TASK (self), error);
}


static void
compute_pages_size (GthContactSheetCreator *self)
{
	int    n_pages;
	int    columns;
	GList *scan;

	self->priv->page_width = self->priv->theme->col_spacing + self->priv->columns_per_page * (self->priv->thumb_width + (self->priv->theme->frame_hpadding * 2) + self->priv->theme->col_spacing);
	self->priv->page_height = 0;

	n_pages = self->priv->n_files / (self->priv->columns_per_page * self->priv->rows_per_page) + 1;
	self->priv->pages_height = g_new (int, n_pages);
	self->priv->n_pages = 0;
	columns = self->priv->columns_per_page;
	for (scan = self->priv->files; scan != NULL; /* void */) {
		int page_height;
		int r;

		page_height = self->priv->theme->row_spacing;

		/* header */

		page_height += get_header_height (self, TRUE);

		/* images */

		for (r = 0; r < self->priv->rows_per_page; r++) {
			GList *first_item;
			GList *last_item;
			int    c;
			int    row_height;

			/* get row items. */

			first_item = scan;
			last_item = NULL;
			for (c = 0; c < columns; c++) {
				if (scan == NULL) {
					columns = c;
					break;
				}
				last_item = scan = scan->next;
			}

			if (columns == 0)
				break;

			row_height = (self->priv->thumb_height
				      + (self->priv->theme->frame_hpadding * 2)
				      + get_max_text_height (self, first_item, last_item)
				      + self->priv->theme->row_spacing);
			page_height += row_height;
		}

		/* footer */

		page_height += get_footer_height (self, TRUE);

		/**/

		self->priv->pages_height[self->priv->n_pages] = page_height;
		self->priv->page_height = MAX (self->priv->page_height, page_height);
		self->priv->n_pages++;
	}
}


static int
item_data_compare_func (gconstpointer a,
			gconstpointer b,
			gpointer      user_data)
{
	GthContactSheetCreator *self = user_data;
	ItemData               *item_data_a = (ItemData *) a;
	ItemData               *item_data_b = (ItemData *) b;
	int                     result;

	result = self->priv->sort_type->cmp_func (item_data_a->file_data, item_data_b->file_data);
	if (self->priv->sort_inverse)
		result = result * -1;

	return result;
}


static void image_loader_ready_cb (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data);


static void
load_current_image (GthContactSheetCreator *self)
{
	ItemData *item_data;

	if (self->priv->current_file == NULL) {
		if (self->priv->sort_type->cmp_func != 0)
			self->priv->files = g_list_sort_with_data (self->priv->files, item_data_compare_func, self);
		compute_pages_size (self);
		export (self);
		return;
	}

	item_data = self->priv->current_file->data;

	gth_task_progress (GTH_TASK (self),
			   _("Generating thumbnails"),
			   g_file_info_get_display_name (item_data->file_data->info),
			   FALSE,
			   ((double) ++self->priv->n_loaded_files) / (self->priv->n_files + 1));

	gth_image_loader_load (self->priv->image_loader,
			       item_data->file_data,
			       -1,
			       G_PRIORITY_DEFAULT,
			       gth_task_get_cancellable (GTH_TASK (self)),
			       image_loader_ready_cb,
			       self);
}


static void
image_loader_ready_cb (GObject      *source_object,
		       GAsyncResult *result,
		       gpointer      user_data)
{
	GthContactSheetCreator *self = user_data;
	GthImage               *image = NULL;
	cairo_surface_t        *image_surface;
	int                     original_width;
	int                     original_height;
	GError                 *error = NULL;
	ItemData               *item_data;

	if (! gth_image_loader_load_finish (GTH_IMAGE_LOADER (source_object),
					    result,
					    &image,
					    &original_width,
					    &original_height,
					    NULL,
					    &error))
	{
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	image_surface = gth_image_get_cairo_surface (image);

	item_data = self->priv->current_file->data;
	if (self->priv->squared_thumbnails) {
		item_data->thumbnail = _cairo_image_surface_scale_squared (image_surface, MIN (self->priv->thumb_height, self->priv->thumb_width), SCALE_FILTER_BEST, NULL);
	}
	else {
		int width, height;

		width = cairo_image_surface_get_width (image_surface);
		height = cairo_image_surface_get_height (image_surface);
		if (scale_keeping_ratio (&width, &height, self->priv->thumb_width, self->priv->thumb_height, FALSE))
			item_data->thumbnail = _cairo_image_surface_scale (image_surface, width, height, SCALE_FILTER_BEST, NULL);
		else
			item_data->thumbnail = cairo_surface_reference (image_surface);
	}
	item_data->original_width = original_width;
	item_data->original_height = original_height;

	cairo_surface_destroy (image_surface);
	g_object_unref (image);

	self->priv->current_file = self->priv->current_file->next;
	load_current_image (self);
}


static void
file_list_info_ready_cb (GList    *files,
			 GError   *error,
			 gpointer  user_data)
{
	GthContactSheetCreator *self = user_data;
	GList                  *scan;

	if (error != NULL) {
		gth_task_completed (GTH_TASK (self), error);
		return;
	}

	self->priv->files = NULL;
	for (scan = files; scan; scan = scan->next)
		self->priv->files = g_list_prepend (self->priv->files, item_data_new ((GthFileData *) scan->data));
	self->priv->files = g_list_reverse (self->priv->files);

	if (self->priv->image_loader == NULL)
		self->priv->image_loader = gth_image_loader_new (NULL, NULL);

	self->priv->current_file = self->priv->files;
	load_current_image (self);
}


static void
gth_contact_sheet_creator_exec (GthTask *task)
{
	GthContactSheetCreator *self = GTH_CONTACT_SHEET_CREATOR (task);
	int                     n_files;
	char                   *required_metadata;

	self->priv->n_files = g_list_length (self->priv->gfile_list);
	self->priv->n_loaded_files = 0;

	n_files = self->priv->single_index ? self->priv->n_files : self->priv->images_per_index;
	self->priv->rows_per_page = n_files / self->priv->columns_per_page;
	if (n_files % self->priv->columns_per_page > 0)
		self->priv->rows_per_page += 1;

	self->priv->pango_context = gdk_pango_context_get ();
	pango_context_set_language (self->priv->pango_context, gtk_get_default_language ());
	self->priv->pango_layout = pango_layout_new (self->priv->pango_context);
	pango_layout_set_alignment (self->priv->pango_layout, PANGO_ALIGN_CENTER);

	required_metadata = g_strconcat (GFILE_STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE, ",", self->priv->thumbnail_caption, NULL);
	_g_query_all_metadata_async (self->priv->gfile_list,
				     GTH_LIST_DEFAULT,
				     required_metadata,
				     gth_task_get_cancellable (GTH_TASK (self)),
				     file_list_info_ready_cb,
				     self);

	g_free (required_metadata);
}


static void
gth_contact_sheet_creator_finalize (GObject *object)
{
	GthContactSheetCreator *self;

	g_return_if_fail (GTH_IS_CONTACT_SHEET_CREATOR (object));

	self = GTH_CONTACT_SHEET_CREATOR (object);
	g_strfreev (self->priv->thumbnail_caption_v);
	g_strfreev (self->priv->template_v);
	g_free (self->priv->pages_height);
	_g_object_unref (self->priv->imagemap_stream);
	_g_object_unref (self->priv->imagemap_file);
	_g_object_list_unref (self->priv->created_files);
	g_list_foreach (self->priv->files, (GFunc) item_data_free, NULL);
	g_list_free (self->priv->files);
	_g_object_unref (self->priv->image_loader);
	_g_object_unref (self->priv->pango_layout);
	_g_object_unref (self->priv->pango_context);
	if (self->priv->cr != NULL)
		cairo_destroy (self->priv->cr);
	g_free (self->priv->thumbnail_caption);
	gth_contact_sheet_theme_unref (self->priv->theme);
	g_free (self->priv->mime_type);
	g_free (self->priv->file_extension);
	g_free (self->priv->template);
	_g_object_unref (self->priv->destination_file);
	_g_object_unref (self->priv->destination);
	g_free (self->priv->footer);
	g_free (self->priv->header);
	_g_object_list_unref (self->priv->gfile_list);
	if (self->priv->timestamp != NULL)
		g_date_time_unref (self->priv->timestamp);
	g_free (self->priv->location_name);

	G_OBJECT_CLASS (gth_contact_sheet_creator_parent_class)->finalize (object);
}


static void
gth_contact_sheet_creator_class_init (GthContactSheetCreatorClass *klass)
{
	GObjectClass *object_class;
	GthTaskClass *task_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gth_contact_sheet_creator_finalize;

	task_class = GTH_TASK_CLASS (klass);
	task_class->exec = gth_contact_sheet_creator_exec;
}


static void
gth_contact_sheet_creator_init (GthContactSheetCreator *self)
{
	self->priv = gth_contact_sheet_creator_get_instance_private (self);
	self->priv->header = NULL;
	self->priv->footer = NULL;
	self->priv->destination = NULL;
	self->priv->destination_file = NULL;
	self->priv->template = NULL;
	self->priv->mime_type = NULL;
	self->priv->file_extension = NULL;
	self->priv->theme = NULL;
	self->priv->pango_context = NULL;
	self->priv->pango_layout = NULL;
	self->priv->image_loader = NULL;
	self->priv->files = NULL;
	self->priv->created_files = NULL;
	self->priv->imagemap_file = NULL;
	self->priv->imagemap_stream = NULL;
	self->priv->pages_height = NULL;
	self->priv->template_v = NULL;
	self->priv->thumbnail_caption_v = NULL;
	self->priv->write_image_map = FALSE;
	self->priv->images_per_index = 0;
	self->priv->single_index = FALSE;
	self->priv->columns_per_page = 0;
	self->priv->rows_per_page = 0;
	self->priv->sort_type = NULL;
	self->priv->sort_inverse = FALSE;
	self->priv->same_size = FALSE;
	self->priv->squared_thumbnails = FALSE;
	self->priv->thumb_width = DEFAULT_THUMB_SIZE;
	self->priv->thumb_height = DEFAULT_THUMB_SIZE;
	self->priv->thumbnail_caption = NULL;
	self->priv->location_name = NULL;
	self->priv->timestamp = NULL;
}


GthTask *
gth_contact_sheet_creator_new (GthBrowser *browser,
			       GList      *file_list)
{
	GthContactSheetCreator *self;

	g_return_val_if_fail (browser != NULL, NULL);

	self = (GthContactSheetCreator *) g_object_new (GTH_TYPE_CONTACT_SHEET_CREATOR, NULL);
	self->priv->browser = browser;
	self->priv->gfile_list = _g_object_list_ref (file_list);

	return (GthTask *) self;
}


void
gth_contact_sheet_creator_set_header (GthContactSheetCreator *self,
				      const char             *value)
{
	_g_str_set (&self->priv->header, value);
}


void
gth_contact_sheet_creator_set_footer (GthContactSheetCreator *self,
				      const char             *value)
{
	_g_str_set (&self->priv->footer, value);
}


void
gth_contact_sheet_creator_set_destination (GthContactSheetCreator *self,
					   GFile                  *destination)
{
	_g_clear_object (&self->priv->destination);
	self->priv->destination = _g_object_ref (destination);
}


void
gth_contact_sheet_creator_set_filename_template (GthContactSheetCreator *self,
						 const char             *filename_template)
{
	_g_str_set (&self->priv->template, filename_template);
	if (self->priv->template_v != NULL)
		g_strfreev (self->priv->template_v);
	self->priv->template_v = _g_template_tokenize (self->priv->template, 0);
}


void
gth_contact_sheet_creator_set_mime_type (GthContactSheetCreator *self,
					 const char             *mime_type,
					 const char             *file_extension)
{
	_g_str_set (&self->priv->mime_type, mime_type);
	_g_str_set (&self->priv->file_extension, file_extension);
}


void
gth_contact_sheet_creator_set_write_image_map (GthContactSheetCreator *self,
					       gboolean                value)
{
	self->priv->write_image_map = value;
}


void
gth_contact_sheet_creator_set_theme (GthContactSheetCreator *self,
				     GthContactSheetTheme   *theme)
{
	if (self->priv->theme != NULL) {
		gth_contact_sheet_theme_unref (self->priv->theme);
		self->priv->theme = NULL;
	}

	if (theme != NULL)
		self->priv->theme = gth_contact_sheet_theme_ref (theme);
}


void
gth_contact_sheet_creator_set_images_per_index (GthContactSheetCreator *self,
					        int                     value)
{
	self->priv->images_per_index = value;
}


void
gth_contact_sheet_creator_set_single_index (GthContactSheetCreator *self,
					    gboolean                value)
{
	self->priv->single_index = value;
}


void
gth_contact_sheet_creator_set_columns (GthContactSheetCreator *self,
			   	       int                     columns)
{
	self->priv->columns_per_page = columns;
}


void
gth_contact_sheet_creator_set_sort_order (GthContactSheetCreator *self,
					  GthFileDataSort        *sort_type,
					  gboolean                sort_inverse)
{
	self->priv->sort_type = sort_type;
	self->priv->sort_inverse = sort_inverse;
}


void
gth_contact_sheet_creator_set_same_size (GthContactSheetCreator *self,
					 gboolean                value)
{
	self->priv->same_size = value;
}


void
gth_contact_sheet_creator_set_thumb_size (GthContactSheetCreator *self,
					  gboolean                squared,
					  int                     width,
					  int                     height)
{
	self->priv->squared_thumbnails = squared;
	self->priv->thumb_width = width;
	self->priv->thumb_height = height;
}


void
gth_contact_sheet_creator_set_thumbnail_caption (GthContactSheetCreator *self,
						 const char             *caption)
{
	_g_str_set (&self->priv->thumbnail_caption, caption);
	self->priv->thumbnail_caption_v = g_strsplit (self->priv->thumbnail_caption, ",", -1);
}


void
gth_contact_sheet_creator_set_location_name (GthContactSheetCreator *self,
					     const char             *name)
{
	_g_str_set (&self->priv->location_name, name);
}
