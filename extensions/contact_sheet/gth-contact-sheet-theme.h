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

#ifndef GTH_CONTACT_SHEET_THEME_H
#define GTH_CONTACT_SHEET_THEME_H

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gthumb.h>

typedef enum {
	GTH_CONTACT_SHEET_BACKGROUND_TYPE_SOLID,
	GTH_CONTACT_SHEET_BACKGROUND_TYPE_VERTICAL,
	GTH_CONTACT_SHEET_BACKGROUND_TYPE_HORIZONTAL,
	GTH_CONTACT_SHEET_BACKGROUND_TYPE_FULL
} GthContactSheetBackgroundType;

typedef enum {
	GTH_CONTACT_SHEET_FRAME_STYLE_NONE,
	GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE,
	GTH_CONTACT_SHEET_FRAME_STYLE_SIMPLE_WITH_SHADOW,
	GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW,
	GTH_CONTACT_SHEET_FRAME_STYLE_SLIDE,
	GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW_IN,
	GTH_CONTACT_SHEET_FRAME_STYLE_SHADOW_OUT
} GthContactSheetFrameStyle;

typedef struct {
	int                            ref;
	GFile                         *file;
	char                          *display_name;

	GthContactSheetBackgroundType  background_type;
	GdkRGBA                        background_color1;
	GdkRGBA                        background_color2;
	GdkRGBA                        background_color3;
	GdkRGBA                        background_color4;

	GthContactSheetFrameStyle      frame_style;
	GdkRGBA                        frame_color;
	int                            frame_hpadding;
	int                            frame_vpadding;
	int                            frame_border;

	char                          *header_font_name;
	GdkRGBA                        header_color;

	char                          *footer_font_name;
	GdkRGBA                        footer_color;

	char                          *caption_font_name;
	GdkRGBA                        caption_color;
	int                            caption_spacing;

	int                            row_spacing;
	int                            col_spacing;

	gboolean                       editable;
} GthContactSheetTheme;

GthContactSheetTheme * gth_contact_sheet_theme_new               (void);
GthContactSheetTheme * gth_contact_sheet_theme_new_from_key_file (GKeyFile              *key_file);
GthContactSheetTheme * gth_contact_sheet_theme_dup               (GthContactSheetTheme  *theme);
gboolean               gth_contact_sheet_theme_to_data           (GthContactSheetTheme  *theme,
								  void                 **buffer,
								  gsize                 *count,
								  GError               **error);
GthContactSheetTheme * gth_contact_sheet_theme_ref               (GthContactSheetTheme  *theme);
void                   gth_contact_sheet_theme_unref             (GthContactSheetTheme  *theme);
void                   gth_contact_sheet_theme_paint_background  (GthContactSheetTheme  *theme,
								  cairo_t               *cr,
								  int                    width,
								  int                    height);
void                   gth_contact_sheet_theme_paint_frame       (GthContactSheetTheme  *theme,
								  cairo_t               *cr,
								  cairo_rectangle_int_t *frame_rect,
								  cairo_rectangle_int_t *image_rect);
void                   gth_contact_sheet_theme_paint_preview     (GthContactSheetTheme  *theme,
								  cairo_t               *cr,
								  int                    width,
								  int                    height);
GdkPixbuf *            gth_contact_sheet_theme_create_preview    (GthContactSheetTheme  *theme,
								  int                    preview_size);

GList *                gth_contact_sheet_theme_list_copy         (GList *list);
void                   gth_contact_sheet_theme_list_free         (GList *list);

#endif /* GTH_CONTACT_SHEET_THEME_H */
