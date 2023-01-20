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
#include "contact-sheet-enum-types.h"
#include "gth-contact-sheet-theme.h"


#define DEFAULT_FONT "Sans 12"
#define FRAME_BORDER 5
#define HEADER_TEXT _("Header")
#define FOOTER_TEXT _("Footer")
#define CAPTION_TEXT _("Caption")


static void
_g_key_file_get_rgba (GKeyFile  *key_file,
		      char      *group_name,
		      char      *key,
		      GdkRGBA   *color,
		      GError   **error)
{
	char *spec;

	spec = g_key_file_get_string (key_file, group_name, key, error);
	if (spec != NULL)
		gdk_rgba_parse (color, spec);

	g_free (spec);
}


static void
_g_key_file_set_rgba (GKeyFile *key_file,
		      char     *group_name,
		      char     *key,
		      GdkRGBA  *color)
{
	char *color_value;

	color_value = gdk_rgba_to_string (color);
	g_key_file_set_string (key_file, group_name, key, color_value);

	g_free (color_value);
}


GthContactSheetTheme *
gth_contact_sheet_theme_new (void)
{
	GthContactSheetTheme *theme;

	theme = g_new0 (GthContactSheetTheme, 1);
	theme->ref = 1;
	theme->frame_hpadding = 8;
	theme->frame_vpadding = 8;
	theme->caption_spacing = 3;
	theme->row_spacing = 15;
	theme->col_spacing = 15;
	theme->frame_border = 0;
	theme->editable = TRUE;

	return theme;
}


GthContactSheetTheme *
gth_contact_sheet_theme_new_from_key_file (GKeyFile *key_file)
{
	GthContactSheetTheme *theme;
	char                 *nick;

	theme = gth_contact_sheet_theme_new ();

	theme->display_name = g_key_file_get_string (key_file, "Theme", "Name", NULL);

	nick = g_key_file_get_string (key_file, "Background", "Type", NULL);
	theme->background_type = _g_enum_type_get_value_by_nick (GTH_TYPE_CONTACT_SHEET_BACKGROUND_TYPE, nick)->value;
	g_free (nick);

	_g_key_file_get_rgba (key_file, "Background", "Color1", &theme->background_color1, NULL);
	_g_key_file_get_rgba (key_file, "Background", "Color2", &theme->background_color2, NULL);
	_g_key_file_get_rgba (key_file, "Background", "Color3", &theme->background_color3, NULL);
	_g_key_file_get_rgba (key_file, "Background", "Color4", &theme->background_color4, NULL);

	nick = g_key_file_get_string (key_file, "Frame", "Style", NULL);
	theme->frame_style = _g_enum_type_get_value_by_nick (GTH_TYPE_CONTACT_SHEET_FRAME_STYLE, nick)->value;
	g_free (nick);

	_g_key_file_get_rgba (key_file, "Frame", "Color", &theme->frame_color, NULL);

	theme->header_font_name = g_key_file_get_string (key_file, "Header", "Font", NULL);
	_g_key_file_get_rgba (key_file, "Header", "Color", &theme->header_color, NULL);

	theme->footer_font_name = g_key_file_get_string (key_file, "Footer", "Font", NULL);
	_g_key_file_get_rgba (key_file, "Footer", "Color", &theme->footer_color, NULL);

	theme->caption_font_name = g_key_file_get_string (key_file, "Caption", "Font", NULL);
	_g_key_file_get_rgba (key_file, "Caption", "Color", &theme->caption_color, NULL);

	return theme;
}


GthContactSheetTheme *
gth_contact_sheet_theme_dup (GthContactSheetTheme  *theme)
{
	GthContactSheetTheme *new_theme = NULL;
	void                 *data;
	gsize                 length;
	GKeyFile             *key_file;

	gth_contact_sheet_theme_to_data (theme, &data, &length, NULL);
	key_file = g_key_file_new ();
	if (g_key_file_load_from_data (key_file, data, length, G_KEY_FILE_NONE, NULL))
		new_theme = gth_contact_sheet_theme_new_from_key_file (key_file);

	g_key_file_free (key_file);

	if (new_theme != NULL) {
		_g_object_unref (new_theme->file);
		new_theme->file = _g_object_ref (theme->file);
	}

	return new_theme;
}


gboolean
gth_contact_sheet_theme_to_data (GthContactSheetTheme  *theme,
				 void                 **buffer,
				 gsize                 *count,
				 GError               **error)
{
	GKeyFile *key_file;

	key_file = g_key_file_new ();
	g_key_file_set_string (key_file, "Theme", "Name", theme->display_name);

	g_key_file_set_string (key_file, "Background", "Type", _g_enum_type_get_value (GTH_TYPE_CONTACT_SHEET_BACKGROUND_TYPE, theme->background_type)->value_nick);
	_g_key_file_set_rgba (key_file, "Background", "Color1", &theme->background_color1);
	if (theme->background_type != GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID) {
		_g_key_file_set_rgba (key_file, "Background", "Color2", &theme->background_color2);
		if (theme->background_type == GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL) {
			_g_key_file_set_rgba (key_file, "Background", "Color3", &theme->background_color3);
			_g_key_file_set_rgba (key_file, "Background", "Color4", &theme->background_color4);
		}
	}

	g_key_file_set_string (key_file, "Frame", "Style", _g_enum_type_get_value (GTH_TYPE_CONTACT_SHEET_FRAME_STYLE, theme->frame_style)->value_nick);
	_g_key_file_set_rgba (key_file, "Frame", "Color", &theme->frame_color);

	g_key_file_set_string (key_file, "Header", "Font", theme->header_font_name);
	_g_key_file_set_rgba (key_file, "Header", "Color", &theme->header_color);

	g_key_file_set_string (key_file, "Footer", "Font", theme->footer_font_name);
	_g_key_file_set_rgba (key_file, "Footer", "Color", &theme->footer_color);

	g_key_file_set_string (key_file, "Caption", "Font", theme->caption_font_name);
	_g_key_file_set_rgba (key_file, "Caption", "Color", &theme->caption_color);

	*buffer = g_key_file_to_data (key_file, count, error);

	g_key_file_free (key_file);

	return *buffer != NULL;
}


GthContactSheetTheme *
gth_contact_sheet_theme_ref (GthContactSheetTheme *theme)
{
	theme->ref++;
	return theme;
}


void
gth_contact_sheet_theme_unref (GthContactSheetTheme *theme)
{
	if (theme == NULL)
		return;

	theme->ref--;
	if (theme->ref > 0)
		return;

	_g_object_unref (theme->file);
	g_free (theme->display_name);
	g_free (theme->header_font_name);
	g_free (theme->footer_font_name);
	g_free (theme->caption_font_name);
	g_free (theme);
}


void
gth_contact_sheet_theme_paint_background (GthContactSheetTheme *theme,
					  cairo_t              *cr,
					  int                   width,
					  int                   height)
{
	cairo_surface_t *surface;
	cairo_pattern_t *pattern;

	switch (theme->background_type) {
	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID:
		gdk_cairo_set_source_rgba (cr, &theme->background_color1);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL:
		pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, theme->background_color1.red, theme->background_color1.green, theme->background_color1.blue, theme->background_color1.alpha);
		cairo_pattern_add_color_stop_rgba (pattern, height, theme->background_color2.red, theme->background_color2.green, theme->background_color2.blue, theme->background_color2.alpha);
		cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
		cairo_set_source (cr, pattern);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL:
		pattern = cairo_pattern_create_linear (0.0, 0.0, width, 0.0);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, theme->background_color1.red, theme->background_color1.green, theme->background_color1.blue, theme->background_color1.alpha);
		cairo_pattern_add_color_stop_rgba (pattern, width, theme->background_color2.red, theme->background_color2.green, theme->background_color2.blue, theme->background_color2.alpha);
		cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
		cairo_set_source (cr, pattern);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		break;

	case GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL:
		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
		_cairo_paint_full_gradient (surface,
					    &theme->background_color1,
					    &theme->background_color2,
					    &theme->background_color3,
					    &theme->background_color4);
		cairo_set_source_surface (cr, surface, 0, 0);
		cairo_rectangle (cr, 0, 0, width, height);
		cairo_fill (cr);
		cairo_surface_destroy (surface);
		break;
	}
}


void
gth_contact_sheet_theme_paint_frame (GthContactSheetTheme  *theme,
				     cairo_t               *cr,
				     cairo_rectangle_int_t *frame_rect,
				     cairo_rectangle_int_t *image_rect)
{
	int width;

	switch (theme->frame_style) {
	case GTH_CONTACT_SHEET_FRAME_STYLE_NONE:
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW:
		_cairo_draw_drop_shadow (cr,
					 image_rect->x,
					 image_rect->y,
					 image_rect->width,
					 image_rect->height,
					 3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE:
		gdk_cairo_set_source_rgba (cr, &theme->frame_color);
		_cairo_draw_frame (cr,
				   image_rect->x,
				   image_rect->y,
				   image_rect->width,
				   image_rect->height,
				   3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE_WITH_SHADOW:
		_cairo_draw_drop_shadow (cr,
					 image_rect->x,
					 image_rect->y,
					 image_rect->width,
					 image_rect->height,
					 5);

		gdk_cairo_set_source_rgba (cr, &theme->frame_color);
		_cairo_draw_frame (cr,
				   image_rect->x,
				   image_rect->y,
				   image_rect->width,
				   image_rect->height,
				   3);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SLIDE:
		_cairo_draw_slide (cr,
				   frame_rect->x,
				   frame_rect->y,
				   frame_rect->width,
				   frame_rect->height,
				   image_rect->width,
				   image_rect->height,
				   &theme->frame_color,
				   TRUE);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW_IN:
		width = 3;
		cairo_save (cr);
		cairo_set_line_width (cr, width);
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
		cairo_move_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_stroke (cr);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.5);
		cairo_move_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_stroke (cr);
		cairo_restore (cr);
		break;

	case GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW_OUT:
		width = 3;
		cairo_save (cr);
		cairo_set_line_width (cr, width);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.5);
		cairo_move_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_stroke (cr);
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
		cairo_move_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y - (width / 2));
		cairo_line_to (cr, image_rect->x + image_rect->width + (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_line_to (cr, image_rect->x - (width / 2), image_rect->y + image_rect->height + (width / 2));
		cairo_stroke (cr);
		cairo_restore (cr);
		break;
	}
}


static void
get_text_bounds (GthContactSheetTheme  *theme,
		 const char            *font_name,
		 int                    width,
		 double                 font_scale,
		 const char            *text,
		 PangoRectangle        *bounds)
{
	PangoContext         *pango_context;
	PangoLayout          *pango_layout;
	PangoFontDescription *font_desc;

	pango_context = gdk_pango_context_get ();
	pango_context_set_language (pango_context, gtk_get_default_language ());
	pango_layout = pango_layout_new (pango_context);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);

	if (font_name != NULL)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);

	if (font_scale != 1.0) {
		double                size_in_points;
		cairo_font_options_t *options;

		size_in_points = (double) pango_font_description_get_size (font_desc) * font_scale;
		pango_font_description_set_absolute_size (font_desc, size_in_points);
		pango_layout_set_font_description (pango_layout, font_desc);

		options = cairo_font_options_create ();
		cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
		pango_cairo_context_set_font_options (pango_context, options);
		cairo_font_options_destroy (options);
	}

	pango_layout_set_font_description (pango_layout, font_desc);
	pango_layout_set_width (pango_layout, width * PANGO_SCALE);
	pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_text (pango_layout, text, -1);
	pango_layout_get_pixel_extents (pango_layout, NULL, bounds);
}


static void
paint_text (GthContactSheetTheme  *theme,
	    cairo_t               *cr,
	    const char            *font_name,
	    GdkRGBA               *color,
	    int                    x,
	    int                    y,
	    int                    width,
	    gboolean               footer,
	    double                 font_scale,
	    const char            *text)
{
	PangoContext         *pango_context;
	PangoLayout          *pango_layout;
	PangoFontDescription *font_desc;
	PangoRectangle        bounds;

	pango_context = gdk_pango_context_get ();
	pango_context_set_language (pango_context, gtk_get_default_language ());
	pango_layout = pango_layout_new (pango_context);
	pango_layout_set_alignment (pango_layout, PANGO_ALIGN_CENTER);

	if (font_name != NULL)
		font_desc = pango_font_description_from_string (font_name);
	else
		font_desc = pango_font_description_from_string (DEFAULT_FONT);

	if (font_scale != 1.0) {
		double                size_in_points;
		cairo_font_options_t *options;

		size_in_points = (double) pango_font_description_get_size (font_desc) * font_scale;
		pango_font_description_set_absolute_size (font_desc, size_in_points);
		pango_layout_set_font_description (pango_layout, font_desc);

		options = cairo_font_options_create ();
		cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
		pango_cairo_context_set_font_options (pango_context, options);
		cairo_font_options_destroy (options);
	}

	pango_layout_set_font_description (pango_layout, font_desc);
	pango_layout_set_width (pango_layout, width * PANGO_SCALE);
	pango_layout_set_wrap (pango_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_text (pango_layout, text, -1);
	pango_layout_get_pixel_extents (pango_layout, NULL, &bounds);

	cairo_save (cr);
	gdk_cairo_set_source_rgba (cr, color);
	pango_cairo_update_layout (cr, pango_layout);
	if (footer)
		cairo_move_to (cr, x, y - bounds.height - 2);
	else
		cairo_move_to (cr, x, y);
	pango_cairo_show_layout (cr, pango_layout);
	cairo_restore (cr);

#if 0
	cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
	cairo_rectangle (cr, x, y, width, bounds.height);
	cairo_stroke (cr);
#endif

	pango_font_description_free (font_desc);
	g_object_unref (pango_layout);
	g_object_unref (pango_context);
}


static void
paint_thumbnail (GthContactSheetTheme  *theme,
		 cairo_t               *cr,
		 cairo_rectangle_int_t *image_rect,
		 double                 font_scale)
{
	cairo_rectangle_int_t frame_rect;

	/* frame */

	frame_rect.x = image_rect->x - FRAME_BORDER;
	frame_rect.y = image_rect->y - FRAME_BORDER;
	frame_rect.width = image_rect->width + (FRAME_BORDER * 2);
	frame_rect.height = image_rect->height + (FRAME_BORDER * 2);
	gth_contact_sheet_theme_paint_frame (theme, cr, &frame_rect, image_rect);

	/* pseduo-image */

	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_rectangle (cr, image_rect->x, image_rect->y, image_rect->width, image_rect->height);
	cairo_fill (cr);

	cairo_set_source_rgb (cr, 0.66, 0.66, 0.66);
	cairo_set_line_width (cr, 1.0);
	cairo_rectangle (cr, image_rect->x + 0.5, image_rect->y + 0.5, image_rect->width - 1, image_rect->height - 1);
	cairo_move_to (cr, image_rect->x + 0.5, image_rect->y + 0.5);
	cairo_line_to (cr, image_rect->x + image_rect->width - 1, image_rect->y + 0.5 + image_rect->height - 1);
	cairo_move_to (cr, image_rect->x + image_rect->width - 1, image_rect->y + 0.5);
	cairo_line_to (cr, image_rect->x + 0.5, image_rect->y + 0.5 + image_rect->height - 1);
	cairo_stroke (cr);

	/* caption */

	paint_text (theme,
		    cr,
		    theme->caption_font_name,
		    &theme->caption_color,
		    frame_rect.x,
		    frame_rect.y + frame_rect.height + 2,
		    frame_rect.width,
		    FALSE,
		    font_scale,
		    CAPTION_TEXT);
}


void
gth_contact_sheet_theme_paint_preview (GthContactSheetTheme  *theme,
				       cairo_t               *cr,
				       int                    width,
				       int                    height)
{
	double                font_scale;
	cairo_rectangle_int_t image_rect;

	if (height < 200)
		font_scale = height / 200.0;
	else
		font_scale = 1.0;

	gth_contact_sheet_theme_paint_background (theme, cr, width, height);

	/* thumbnail */

	if (height < 200) {
		image_rect.width = width / 2;
		image_rect.height = image_rect.width;
		image_rect.x = (width - image_rect.width) / 2;
		image_rect.y = (height - image_rect.height) / 2 - 3;
		paint_thumbnail (theme, cr, &image_rect, font_scale);
	}
	else {
		PangoRectangle header_rect;
		PangoRectangle footer_rect;
		PangoRectangle caption_rect;
		int            n_columns;
		int            n_rows;
		int            size;
		int            x_offset;
		int            y_offset;
		int            c, r;

		size = 80;

		get_text_bounds (theme,
				 theme->header_font_name,
				 width,
				 font_scale,
				 HEADER_TEXT,
				 &header_rect);
		get_text_bounds (theme,
				 theme->footer_font_name,
				 width,
				 font_scale,
				 FOOTER_TEXT,
				 &footer_rect);
		get_text_bounds (theme,
				 theme->caption_font_name,
				 size,
				 font_scale,
				 CAPTION_TEXT,
				 &caption_rect);

		n_columns = ((width- (theme->col_spacing * 2)) / (size + (FRAME_BORDER * 2) + theme->col_spacing));
		n_rows = ((height - header_rect.height - (theme->row_spacing * 2) - footer_rect.height) / (size + theme->col_spacing + caption_rect.height));
		x_offset = (width - (n_columns * (size + theme->col_spacing))) / 2;
		y_offset = header_rect.height + theme->row_spacing;
		for (r = 0; r < n_rows; r++) {
			int y;

			y = y_offset + r * (size + caption_rect.height + theme->row_spacing);
			for (c = 0; c < n_columns; c++) {
				image_rect.width = size;
				image_rect.height = size;
				image_rect.x = x_offset + c * (size + theme->col_spacing);
				image_rect.y = y;
				paint_thumbnail (theme, cr, &image_rect, font_scale);
			}
		}
	}

	/* header */

	paint_text (theme,
		    cr,
		    theme->header_font_name,
		    &theme->header_color,
		    0,
		    0,
		    width,
		    FALSE,
		    font_scale,
		    HEADER_TEXT);

	/* footer */

	paint_text (theme,
		    cr,
		    theme->footer_font_name,
		    &theme->footer_color,
		    0,
		    height,
		    width,
		    TRUE,
		    font_scale,
		    FOOTER_TEXT);
}


GdkPixbuf *
gth_contact_sheet_theme_create_preview (GthContactSheetTheme *theme,
					int                   preview_size)
{
	cairo_surface_t *surface;
	cairo_t         *cr;
	GdkPixbuf       *pixbuf;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, preview_size, preview_size);
	cr = cairo_create (surface);
	cairo_surface_destroy (surface);
	gth_contact_sheet_theme_paint_preview (theme, cr, preview_size, preview_size);
	pixbuf = _gdk_pixbuf_new_from_cairo_context (cr);

	cairo_destroy (cr);

	return pixbuf;
}


GList *
gth_contact_sheet_theme_list_copy (GList *list)
{
	GList *new_list;

	new_list = g_list_copy (list);
	g_list_foreach (new_list, (GFunc) gth_contact_sheet_theme_ref, NULL);

	return new_list;
}


void
gth_contact_sheet_theme_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) gth_contact_sheet_theme_unref, NULL);
	g_list_free (list);
}
