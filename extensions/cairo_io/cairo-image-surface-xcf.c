/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
#include <gio/gio.h>
#include <gthumb.h>
#include "cairo-image-surface-xcf.h"


#define TILE_WIDTH	64
#define MAX_TILE_SIZE	(TILE_WIDTH * TILE_WIDTH * 4 * 1.5)
#define DISSOLVE_SEED	737893334


typedef enum {
	GIMP_RGB,
	GIMP_GRAY,
	GIMP_INDEXED
} GimpImageBaseType;


typedef enum {
	GIMP_COMPRESSION_NONE,
	GIMP_COMPRESSION_RLE,
	GIMP_COMPRESSION_ZLIB,
	GIMP_COMPRESSION_FRACTAL
} GimpCompression;


typedef enum {
	GIMP_RGB_IMAGE,
	GIMP_RGBA_IMAGE,
	GIMP_GRAY_IMAGE,
	GIMP_GRAYA_IMAGE,
	GIMP_INDEXED_IMAGE,
	GIMP_INDEXEDA_IMAGE
} GimpImageType;


typedef enum {
	GIMP_LAYER_MODE_NORMAL,
	GIMP_LAYER_MODE_DISSOLVE, /* (random dithering to discrete alpha) */
	GIMP_LAYER_MODE_BEHIND, /* (not selectable in the GIMP UI) */
	GIMP_LAYER_MODE_MULTIPLY,
	GIMP_LAYER_MODE_SCREEN,
	GIMP_LAYER_MODE_OVERLAY,
	GIMP_LAYER_MODE_DIFFERENCE,
	GIMP_LAYER_MODE_ADDITION,
	GIMP_LAYER_MODE_SUBTRACT,
	GIMP_LAYER_MODE_DARKEN_ONLY,
	GIMP_LAYER_MODE_LIGHTEN_ONLY,
	GIMP_LAYER_MODE_HUE, /* (H of HSV) */
	GIMP_LAYER_MODE_SATURATION, /* (S of HSV) */
	GIMP_LAYER_MODE_COLOR, /* (H and S of HSL) */
	GIMP_LAYER_MODE_VALUE, /* (V of HSV) */
	GIMP_LAYER_MODE_DIVIDE,
	GIMP_LAYER_MODE_DODGE,
	GIMP_LAYER_MODE_BURN,
	GIMP_LAYER_MODE_HARD_LIGHT,
	GIMP_LAYER_MODE_SOFT_LIGHT, /* (XCF version >= 2 only) */
	GIMP_LAYER_MODE_GRAIN_EXTRACT, /* (XCF version >= 2 only) */
	GIMP_LAYER_MODE_GRAIN_MERGE /* (XCF version >= 2 only) */
} GimpLayerMode;


typedef struct {
	int             n;
	guint           width;
	guint           height;
	GimpImageType   type;
	char           *name;
	guint32         opacity;
	gboolean        visible;
	gboolean        floating_selection;
	GimpLayerMode   mode;
	gboolean        apply_mask;
	gint32          h_offset;
	gint32          v_offset;
	int             stride;
	guchar         *pixels;
	guchar         *alpha_mask;
	int             bpp;
	struct {
		gboolean   dirty;
		int        rows;
		int        columns;
		int        n_tiles;
		int        last_row_height;
		int        last_col_width;
	} tiles;
} GimpLayer;


typedef struct {
	guchar color1;
	guchar color2;
	guchar color3;
} GimpColormap;


static int cairo_rgba[4]  = { CAIRO_RED, CAIRO_GREEN, CAIRO_BLUE, CAIRO_ALPHA };
static int cairo_graya[2] = { 0, CAIRO_ALPHA };
static int cairo_indexed[2] = { 0, CAIRO_ALPHA };


/* -- GDataInputStream functions -- */


static char *
_g_data_input_stream_read_c_string (GDataInputStream  *stream,
				    gsize              size,
				    GCancellable      *cancellable,
				    GError           **error)
{
	char *string;
	gsize bytes_read;

	g_return_val_if_fail (size > 0, NULL);

	string = g_new (char, size + 1);
	if (g_input_stream_read_all (G_INPUT_STREAM (stream),
				     string,
				     size,
				     &bytes_read,
				     cancellable,
				     error))
	{
		string[bytes_read] = 0;
	}
	else
		string[0] = 0;

	return string;
}


static char *
_g_data_input_stream_read_xcf_string (GDataInputStream  *stream,
				      GCancellable      *cancellable,
				      GError           **error)
{
	guint32 n_bytes;

	n_bytes = g_data_input_stream_read_uint32 (stream, cancellable, error);
	if (n_bytes == 0)
		return NULL;

	return _g_data_input_stream_read_c_string (stream, n_bytes, cancellable, error);
}


/* -- GimpLayer -- */


static GimpLayer *
gimp_layer_new (int n)
{
	GimpLayer *layer;

	layer = g_new0 (GimpLayer, 1);
	layer->n = n;
	layer->width = 0;
	layer->height = 0;
	layer->type = GIMP_RGBA_IMAGE;
	layer->name = NULL;
	layer->opacity = 255;
	layer->visible = TRUE;
	layer->floating_selection = FALSE;
	layer->mode = GIMP_LAYER_MODE_NORMAL;
	layer->apply_mask = FALSE;
	layer->h_offset = 0;
	layer->v_offset = 0;
	layer->tiles.dirty = TRUE;
	layer->tiles.n_tiles = 0;
	layer->pixels = NULL;
	layer->alpha_mask = NULL;

	return layer;
}


static gboolean
gimp_layer_get_tile_size (GimpLayer *layer,
			  int        n_tile,
			  int        bpp,
			  goffset   *offset,
			  int       *width,
			  int       *height)
{
	int   tile_row, tile_column;
	gsize tile_width;
	gsize tile_height;

	if (layer->tiles.dirty) {
		layer->tiles.last_col_width = layer->width % TILE_WIDTH;
		layer->tiles.last_row_height = layer->height % TILE_WIDTH;

		layer->tiles.columns = layer->width / TILE_WIDTH;
		if (layer->tiles.last_col_width > 0)
			layer->tiles.columns++;
		else
			layer->tiles.last_col_width = TILE_WIDTH;

		layer->tiles.rows = layer->height / TILE_WIDTH;
		if (layer->tiles.last_row_height > 0)
			layer->tiles.rows++;
		else
			layer->tiles.last_row_height = TILE_WIDTH;

		layer->tiles.n_tiles = layer->tiles.columns * layer->tiles.rows;
		layer->tiles.dirty = FALSE;
		layer->stride = layer->width * bpp;
	}

	if ((n_tile < 0) || (n_tile >= layer->tiles.n_tiles))
		return FALSE;

	tile_column = (n_tile % layer->tiles.columns);
	if (tile_column == layer->tiles.columns - 1)
		tile_width = layer->tiles.last_col_width;
	else
		tile_width = TILE_WIDTH;

	tile_row = (n_tile / layer->tiles.columns);
	if (tile_row == layer->tiles.rows - 1)
		tile_height = layer->tiles.last_row_height;
	else
		tile_height = TILE_WIDTH;

	*offset = ((tile_row * TILE_WIDTH) * layer->stride) + (tile_column * TILE_WIDTH * bpp);
	*width = tile_width;
	*height = tile_height;

	return TRUE;
}


static void
gimp_layer_free (GimpLayer *layer)
{
	if (layer == NULL)
		return;
	g_free (layer->pixels);
	g_free (layer->alpha_mask);
	g_free (layer->name);
	g_free (layer);
}


/* -- _cairo_image_surface_create_from_xcf -- */


static void
_cairo_image_surface_paint_layer (cairo_surface_t *image,
				  GimpLayer       *layer)
{
	int     image_width;
	int     image_height;
	int     image_row_stride;
	guchar *image_row;
	int     layer_width;
	int     layer_height;
	int     layer_row_stride;
	guchar *layer_row;
	guchar *mask_row;
	int     x, y, width, height;
	guchar *image_pixel;
	guchar *layer_pixel;
	guchar *mask_pixel;
	GRand  *rand_gen;
	int     i, j;
	guchar  r, g, b, a;
	int     temp, temp2;
	guchar  image_hue, image_sat, image_val, image_lum;
	guchar  layer_hue, layer_sat, layer_val, layer_lum;

	if ((image == NULL) || (layer->pixels == NULL))
		return;

	image_width = cairo_image_surface_get_width (image);
	image_height = cairo_image_surface_get_height (image);
	image_row_stride = cairo_image_surface_get_stride (image);

	layer_width = layer->width;
	layer_height = layer->height;
	layer_row_stride = layer->width * 4;

	/* compute the layer <-> image intersection */

	{
		cairo_region_t        *region;
		cairo_rectangle_int_t  rect;

		rect.x = 0;
		rect.y = 0;
		rect.width = image_width;
		rect.height = image_height;
		region = cairo_region_create_rectangle (&rect);

		rect.x = layer->h_offset;
		rect.y = layer->v_offset;
		rect.width = layer_width;
		rect.height = layer_height;
		cairo_region_intersect_rectangle (region, &rect);
		cairo_region_get_extents (region, &rect);
		cairo_region_destroy (region);

		if ((rect.width == 0) || (rect.height == 0))
			return;

		x = rect.x;
		y = rect.y;
		width = rect.width;
		height = rect.height;
	}

	image_row = _cairo_image_surface_flush_and_get_data (image) + (y * image_row_stride) + (x * 4);

	x = (layer->h_offset < 0) ? -layer->h_offset : 0;
	y = (layer->v_offset < 0) ? -layer->v_offset : 0;
	layer_row = layer->pixels + (y * layer_row_stride) + (x * 4);

	mask_row = layer->alpha_mask + (y * layer_width) + x;

	rand_gen = NULL;
	if (layer->mode == GIMP_LAYER_MODE_DISSOLVE)
		rand_gen = g_rand_new_with_seed (DISSOLVE_SEED);

	for (i = 0; i < height; i++) {
		image_pixel = image_row;
		layer_pixel = layer_row;
		mask_pixel = mask_row;

		for (j = 0; j < width; j++) {
			a = ((layer->bpp == 2) || (layer->bpp == 4)) ? layer_pixel[CAIRO_ALPHA] : 255;

			a = ADD_ALPHA (a, layer->opacity);
			if (layer->alpha_mask && (layer->alpha_mask != NULL))
				a = ADD_ALPHA (a, mask_pixel[0]);

			if (a == 0)
				goto next_pixel;

			switch (layer->mode) {
			case GIMP_LAYER_MODE_NORMAL:
			default:
				r = layer_pixel[CAIRO_RED];
				g = layer_pixel[CAIRO_GREEN];
				b = layer_pixel[CAIRO_BLUE];
				break;

			case GIMP_LAYER_MODE_DISSOLVE:
				if (g_rand_int_range (rand_gen, 0, 256) > a)
					goto next_pixel;
				r = layer_pixel[CAIRO_RED];
				g = layer_pixel[CAIRO_GREEN];
				b = layer_pixel[CAIRO_BLUE];
				a = 255;
				break;

			case GIMP_LAYER_MODE_LIGHTEN_ONLY:
				r = GIMP_OP_LIGHTEN_ONLY (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_LIGHTEN_ONLY (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_LIGHTEN_ONLY (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_SCREEN:
				r = GIMP_OP_SCREEN (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_SCREEN (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_SCREEN (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_DODGE:
				r = GIMP_OP_DODGE (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_DODGE (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_DODGE (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_ADDITION:
				r = GIMP_OP_ADDITION (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_ADDITION (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_ADDITION (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_DARKEN_ONLY:
				r = GIMP_OP_DARKEN_ONLY (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_DARKEN_ONLY (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_DARKEN_ONLY (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_MULTIPLY:
				r = GIMP_OP_MULTIPLY (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_MULTIPLY (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_MULTIPLY (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_BURN:
				r = GIMP_OP_BURN (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_BURN (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_BURN (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_OVERLAY:
			case GIMP_LAYER_MODE_SOFT_LIGHT:
				r = GIMP_OP_SOFT_LIGHT (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_SOFT_LIGHT (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_SOFT_LIGHT (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_HARD_LIGHT:
				r = GIMP_OP_HARD_LIGHT (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_HARD_LIGHT (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_HARD_LIGHT (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_DIFFERENCE:
				r = GIMP_OP_DIFFERENCE (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_DIFFERENCE (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_DIFFERENCE (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_SUBTRACT:
				r = GIMP_OP_SUBTRACT (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_SUBTRACT (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_SUBTRACT (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_GRAIN_EXTRACT:
				r = GIMP_OP_GRAIN_EXTRACT (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_GRAIN_EXTRACT (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_GRAIN_EXTRACT (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_GRAIN_MERGE:
				r = GIMP_OP_GRAIN_MERGE (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_GRAIN_MERGE (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_GRAIN_MERGE (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_DIVIDE:
				r = GIMP_OP_DIVIDE (layer_pixel[CAIRO_RED], image_pixel[CAIRO_RED]);
				g = GIMP_OP_DIVIDE (layer_pixel[CAIRO_GREEN], image_pixel[CAIRO_GREEN]);
				b = GIMP_OP_DIVIDE (layer_pixel[CAIRO_BLUE], image_pixel[CAIRO_BLUE]);
				break;

			case GIMP_LAYER_MODE_HUE:
			case GIMP_LAYER_MODE_SATURATION:
			case GIMP_LAYER_MODE_VALUE:
				gimp_rgb_to_hsv (image_pixel[CAIRO_RED],
						 image_pixel[CAIRO_GREEN],
						 image_pixel[CAIRO_BLUE],
						 &image_hue,
						 &image_sat,
						 &image_val);
				gimp_rgb_to_hsv (layer_pixel[CAIRO_RED],
						 layer_pixel[CAIRO_GREEN],
						 layer_pixel[CAIRO_BLUE],
						 &layer_hue,
						 &layer_sat,
						 &layer_val);

				switch (layer->mode) {
				case GIMP_LAYER_MODE_HUE:
					gimp_hsv_to_rgb (layer_hue,
							 image_sat,
							 image_val,
							 &r,
							 &g,
							 &b);
					break;
				case GIMP_LAYER_MODE_SATURATION:
					gimp_hsv_to_rgb (image_hue,
							 layer_sat,
							 image_val,
							 &r,
							 &g,
							 &b);
					break;
				case GIMP_LAYER_MODE_VALUE:
					gimp_hsv_to_rgb (image_hue,
							 image_sat,
							 layer_val,
							 &r,
							 &g,
							 &b);
					break;
				default:
					g_assert_not_reached ();
					break;
				}
				break;

			case GIMP_LAYER_MODE_COLOR:
				gimp_rgb_to_hsl (image_pixel[CAIRO_RED],
						 image_pixel[CAIRO_GREEN],
						 image_pixel[CAIRO_BLUE],
						 &image_hue,
						 &image_sat,
						 &image_lum);
				gimp_rgb_to_hsl (layer_pixel[CAIRO_RED],
						 layer_pixel[CAIRO_GREEN],
						 layer_pixel[CAIRO_BLUE],
						 &layer_hue,
						 &layer_sat,
						 &layer_lum);
				gimp_hsl_to_rgb (layer_hue,
						 layer_sat,
						 image_lum,
						 &r,
						 &g,
						 &b);
				break;
			}

			image_pixel[CAIRO_RED] = GIMP_OP_NORMAL (r, image_pixel[CAIRO_RED], a);
			image_pixel[CAIRO_GREEN] = GIMP_OP_NORMAL (g, image_pixel[CAIRO_GREEN], a);
			image_pixel[CAIRO_BLUE] = GIMP_OP_NORMAL (b, image_pixel[CAIRO_BLUE], a);
			image_pixel[CAIRO_ALPHA] = GIMP_OP_NORMAL (255, image_pixel[CAIRO_ALPHA], a);

next_pixel:

			image_pixel += 4;
			layer_pixel += 4;
			mask_pixel += 1;
		}

		image_row += image_row_stride;
		layer_row += layer_row_stride;
		mask_row += layer_width;
	}

	if (layer->mode == GIMP_LAYER_MODE_DISSOLVE)
		g_rand_free (rand_gen);

	cairo_surface_mark_dirty (image);
}


static cairo_surface_t *
_cairo_image_surface_create_from_layers (int                canvas_width,
					 int                canvas_height,
					 GimpImageBaseType  base_type,
					 GList             *layers)
{
	cairo_surface_t *image;
	GList           *scan;

	image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, canvas_width, canvas_height);

	for (scan = layers; scan; scan = scan->next) {
		GimpLayer *layer = scan->data;

		if (layer->pixels == NULL)
			continue;

		/* the bottommost layer only supports NORMAL and DISSOLVE */

		if ((scan == layers) && (layer->mode != GIMP_LAYER_MODE_DISSOLVE))
			layer->mode = GIMP_LAYER_MODE_NORMAL;

		/* indexed images only support NORMAL and DISSOLVE */

		if ((base_type == GIMP_INDEXED) && (layer->mode != GIMP_LAYER_MODE_DISSOLVE))
			layer->mode = GIMP_LAYER_MODE_NORMAL;

		/* modes supported only by RGB images */

		if (base_type != GIMP_RGB) {
			if ((layer->mode == GIMP_LAYER_MODE_HUE)
			    || (layer->mode == GIMP_LAYER_MODE_SATURATION)
			    || (layer->mode == GIMP_LAYER_MODE_COLOR)
			    || (layer->mode == GIMP_LAYER_MODE_VALUE))
			{
				layer->mode = GIMP_LAYER_MODE_NORMAL;
			}
		}

		_cairo_image_surface_paint_layer (image, layer);

		performance (DEBUG_INFO, "end paint layer %d, mode %d", layer->n, layer->mode);
	}

	return image;
}


static guchar *
read_pixels_from_hierarchy (GDataInputStream  *data_stream,
			    guint32            hierarchy_offset,
			    GimpLayer         *layer,
			    GimpColormap      *colormap,
			    GimpImageBaseType  base_type,
			    GimpCompression    compression,
			    gboolean           is_gimp_channel,
			    GCancellable      *cancellable,
			    GError           **error)
{
	guchar   *image_pixels = NULL;
	guint32   width;
	guint32   height;
	guint32   in_bpp;
	guint32   out_bpp;
	int       row_stride;
	guint32   level_offset;
	GArray   *tile_offsets = NULL;
	guint32   tile_offset;
	guint32   last_tile_offset;
	int       n_tiles;
	int       t;

	/* read the hierarchy structure */

	if (! g_seekable_seek (G_SEEKABLE (data_stream),
			       hierarchy_offset,
			       G_SEEK_SET,
			       cancellable,
			       error))
	{
		return NULL;
	}

	width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	in_bpp = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	level_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	if (is_gimp_channel)
		g_assert (in_bpp == 1);

	if (! is_gimp_channel)
		layer->bpp = in_bpp;

	layer->tiles.dirty = TRUE;

	/* read the level structure */

	if (! g_seekable_seek (G_SEEKABLE (data_stream),
			       level_offset,
			       G_SEEK_SET,
			       cancellable,
			       error))
	{
		goto read_error;
	}

	width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto read_error;

	/* tiles */

	out_bpp = is_gimp_channel ? 1 : 4;
	row_stride = width * out_bpp;
	image_pixels = g_new (guchar, row_stride * height);

	tile_offsets = g_array_new (FALSE, FALSE, sizeof (guint32));;
	n_tiles = 0;
	last_tile_offset = 0;
	while ((tile_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0) {
		n_tiles += 1;
		last_tile_offset = tile_offset;
		g_array_append_val (tile_offsets, tile_offset);
	}
	tile_offset = last_tile_offset + MAX_TILE_SIZE;
	g_array_append_val (tile_offsets, tile_offset);

	if (*error != NULL)
		goto read_error;

	if (compression == GIMP_COMPRESSION_RLE) {
		guchar *tile_data = NULL;

		/* go to the first tile */

		tile_offset = g_array_index (tile_offsets, guint32, 0);
		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       tile_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto read_error;
		}

		/* decompress the tile data */

		tile_data = g_malloc (MAX_TILE_SIZE);

		for (t = 0; t < n_tiles; t++) {
			goffset  tile_data_size;
			guchar  *tile_data_p;
			guchar  *tile_data_limit;
			gsize    data_read;
			goffset  tile_pixels_offset;
			gsize    tile_pixels_size;
			int      tile_width;
			int      tile_height;
			int      c;

			/* read the tile data */

			tile_data_size = g_array_index (tile_offsets, guint32, t + 1) - g_array_index (tile_offsets, guint32, t);
			if (tile_data_size <= 0)
				continue;

			if (! g_input_stream_read_all (G_INPUT_STREAM (data_stream),
						       tile_data,
						       tile_data_size,
						       &data_read,
						       cancellable,
						       error))
			{
				goto rle_error;
			}

			/* decompress the channel streams */

			if (! gimp_layer_get_tile_size (layer,
							t,
							out_bpp,
							&tile_pixels_offset,
							&tile_width,
							&tile_height))
			{
				goto rle_error;
			}

			tile_pixels_size = tile_width * tile_height;
			tile_data_p = tile_data;
			tile_data_limit = tile_data + data_read - 1;

			for (c = 0; c < in_bpp; c++) {
				int     channel_offset;
				guchar *pixels_row;
				guchar *pixel;
				int     size;
				int     n, p, q, v;
				int     tile_column;

				if (is_gimp_channel)
					channel_offset = 0;
				else if (base_type == GIMP_INDEXED)
					channel_offset = cairo_indexed[c];
				else if (in_bpp >= 3)
					channel_offset = cairo_rgba[c];
				else if (in_bpp <= 2)
					channel_offset = cairo_graya[c];
				else
					channel_offset = 0;
				pixels_row = image_pixels + tile_pixels_offset + channel_offset;
				pixel = pixels_row;

				size = tile_pixels_size;
				tile_column = 0;

#define SET_PIXEL(v) {							\
        tile_column++;							\
        if (tile_column > tile_width) {					\
                pixels_row += row_stride;				\
                pixel = pixels_row;					\
                tile_column = 1;					\
        }								\
	if ((base_type == GIMP_INDEXED) && (c == 0)) {			\
		guchar *color = (guchar *) (colormap + (v));		\
		pixel[CAIRO_RED] = color[0];				\
		pixel[CAIRO_GREEN] = color[1];				\
		pixel[CAIRO_BLUE] = color[2];				\
	}								\
	else if (! is_gimp_channel && (in_bpp <= 2) && (c == 0)) {	\
		pixel[CAIRO_RED] = (v);					\
		pixel[CAIRO_GREEN] = (v);				\
		pixel[CAIRO_BLUE] = (v);				\
	}								\
	else								\
		*pixel = (v);						\
	pixel += out_bpp;						\
}

				while (size > 0) {
					if (tile_data_p > tile_data_limit)
						goto rle_error;

					n = *tile_data_p++;

					if ((n >= 0) && (n <= 127)) {
						/* byte          n     For 0 <= n <= 126: a short run of identical bytes
  	  	  	  	  	  	 * byte          v     Repeat this value n+1 times
						 */

						/* byte          127   A long run of identical bytes
						 * byte          p
						 * byte          q
						 * byte          v     Repeat this value p*256 + q times
						 */

						if (n == 127) {
							if (tile_data_p + 2 > tile_data_limit)
								goto rle_error;
							p = *tile_data_p++;
							q = *tile_data_p++;
							v = *tile_data_p++;
							n = (p * 256) + q;
						}
						else {
							if (tile_data_p > tile_data_limit)
								goto rle_error;
							v = *tile_data_p++;
							n++;
						}

						size -= n;
						if (size < 0)
							goto rle_error;

						while (n-- > 0)
							SET_PIXEL (v);
					}
					else if ((n >= 128) && (n <= 255)) {
						/* byte          128   A long run of different bytes
						 * byte          p
						 * byte          q
						 * byte[p*256+q] data  Copy these verbatim to the output stream */

						/* byte          n     For 129 <= n <= 255: a short run of different bytes
						 * byte[256-n]   data  Copy these verbatim to the output stream */

						if (n == 128) {
							if (tile_data_p + 1 > tile_data_limit)
								goto rle_error;
							p = *tile_data_p++;
							q = *tile_data_p++;
							n = (p * 256) + q;
						}
						else
							n = 256 - n;

						if (tile_data_p + n - 1 > tile_data_limit)
							goto rle_error;

						size -= n;
						if (size < 0)
							goto rle_error;

						while (n-- > 0) {
							v = *tile_data_p++;
							SET_PIXEL (v);
						}
					}
				}
			}

		}

rle_error:

		g_free (tile_data);
	}
	else if (compression == GIMP_COMPRESSION_NONE) {

		/* Gimp doesn't save in uncompressed mode. */

	}

	performance (DEBUG_INFO, "end read hierarchy");

	g_array_free (tile_offsets, TRUE);

	return image_pixels;

read_error:

	g_free (image_pixels);
	g_array_free (tile_offsets, TRUE);

	return NULL;
}


#undef SET_PIXEL


GthImage *
_cairo_image_surface_create_from_xcf (GInputStream  *istream,
				      GthFileData   *file_data,
				      int            requested_size,
				      int           *original_width,
				      int           *original_height,
				      gboolean      *loaded_original,
				      gpointer       user_data,
				      GCancellable  *cancellable,
				      GError       **error)
{
	GthImage          *image = NULL;
	cairo_surface_t   *surface;
	GDataInputStream  *data_stream;
	char              *file_type;
	char              *version;
	guint32            canvas_width;
	guint32            canvas_height;
	GimpImageBaseType  base_type;
	guint32            property_type;
	guint32            payload_length;
	GimpCompression    compression;
	GList             *layers;
	guint32            n_colors;
	GimpColormap      *colormap;
	guint              n_properties;
	gboolean           read_properties;
	GArray            *layer_offsets;
	guint32            layer_offset;
	guint              n_layers;
	guint32            channel_offset;
	guint              n_channels;
	int                i;

	performance (DEBUG_INFO, "start loading");

	gimp_op_init ();

	performance (DEBUG_INFO, "end init");

	compression = GIMP_COMPRESSION_RLE;
	layers = NULL;
	layer_offsets = NULL;
	colormap = NULL;

	data_stream = g_data_input_stream_new (istream);
	g_data_input_stream_set_byte_order (data_stream, G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

	/* file type magic */

	file_type = _g_data_input_stream_read_c_string (data_stream, 9, cancellable, error);
	if (*error != NULL)
		goto out;

	if (g_strcmp0 (file_type, "gimp xcf ") != 0) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid format");
		return NULL;
	}
	g_free (file_type);

	/* version */

	version = _g_data_input_stream_read_c_string (data_stream, 5, cancellable, error);
	if (*error != NULL)
		goto out;
	g_free (version);

	/* canvas size */

	canvas_width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	canvas_height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	/* base type */

	base_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
	if (*error != NULL)
		goto out;

	/* properties */

	read_properties = TRUE;
	n_properties = 0;
	while (read_properties) {
		property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		n_properties += 1;

		switch (property_type) {
		case 0: /* PROP_END */
			read_properties = FALSE;
			break;

		case 1: /* PROP_COLORMAP */
			n_colors = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			if (base_type == GIMP_INDEXED) {
				int     i;
				guchar *c;

				colormap = g_new (GimpColormap, n_colors);
				c = (guchar *) colormap;
				for (i = 0; i < n_colors; i++) {
					c[0] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c[1] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c[2] = g_data_input_stream_read_byte (data_stream, cancellable, error);
					if (*error != NULL)
						goto out;

					c += 3;
				}
			}
			else { /* when skipping the colormap do not trust the payload_length value. */
				g_input_stream_skip (G_INPUT_STREAM (data_stream), (n_colors * 3), cancellable, error);
				if (*error != NULL)
					goto out;
			}
			break;

		case 17: /* PROP_COMPRESSION */
			compression = g_data_input_stream_read_byte (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;
			break;

		default:
			g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
			if (*error != NULL)
				goto out;
			break;
		}
	}

	/* layers */

	n_layers = 0;
	layer_offsets = g_array_new (FALSE, FALSE, sizeof (guint32));
	while ((layer_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0) {
		n_layers += 1;
		g_array_append_val (layer_offsets, layer_offset);
	}

	/* channels */

	n_channels = 0;
	while ((channel_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error)) != 0)
		n_channels += 1;

	if (*error != NULL)
		goto out;

	/* read the layers */

	performance (DEBUG_INFO, "start read layers");

	for (i = 0; i < n_layers; i++) {
		GimpLayer *layer;
		guint32    hierarchy_offset;
		guint32    mask_offset;
		char      *mask_name;

		layer_offset = g_array_index (layer_offsets, guint32, i);

		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       layer_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto out;
		}

		layer = gimp_layer_new (i);
		layers = g_list_prepend (layers, layer);

		/* size */

		layer->width = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		layer->height = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* type  */

		layer->type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* name */

		layer->name = _g_data_input_stream_read_xcf_string (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* properties */

		read_properties = TRUE;
		n_properties = 0;
		while (read_properties) {
			property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			n_properties += 1;

			switch (property_type) {
			case 0: /* PROP_END */
				read_properties = FALSE;
				break;

			case 5: /* PROP_FLOATING_SELECTION */
				layer->floating_selection = TRUE;
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				break;

			case 6: /* PROP_OPACITY */
				layer->opacity = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				break;

			case 7: /* PROP_MODE */
				layer->mode = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				break;

			case 8: /* PROP_VISIBLE */
				layer->visible = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				break;

			case 11: /* PROP_APPLY_MASK */
				layer->apply_mask = (g_data_input_stream_read_uint32 (data_stream, cancellable, error) == 1);
				break;

			case 15: /* PROP_OFFSETS */
				layer->h_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				if (*error != NULL)
					goto out;

				layer->v_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
				break;

			default:
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				break;
			}

			if (*error != NULL)
				goto out;
		}

		/* hierarchy structure offset */

		hierarchy_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* layer mask offset */

		mask_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/* the layer image */

		if (layer->floating_selection || ! layer->visible || (layer->opacity == 0))
			continue;

		layer->pixels = read_pixels_from_hierarchy (data_stream, hierarchy_offset, layer, colormap, base_type, compression, FALSE, cancellable, error);
		if (*error != NULL)
			goto out;

		/* read the mask  */

		if (! layer->apply_mask || (mask_offset == 0))
			continue;

		if (! g_seekable_seek (G_SEEKABLE (data_stream),
				       mask_offset,
				       G_SEEK_SET,
				       cancellable,
				       error))
		{
			goto out;
		}

		/* mask width, height and name */

		/*mask_width = */ g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		/*mask_height = */ g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		mask_name = _g_data_input_stream_read_xcf_string (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		g_free (mask_name);

		/* mask properties */

		read_properties = TRUE;
		n_properties = 0;
		while (read_properties) {
			property_type = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			payload_length = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
			if (*error != NULL)
				goto out;

			n_properties += 1;

			switch (property_type) {
			case 0: /* PROP_END */
				read_properties = FALSE;
				break;

			default:
				g_input_stream_skip (G_INPUT_STREAM (data_stream), payload_length, cancellable, error);
				if (*error != NULL)
					goto out;
				break;
			}
		}

		/* the mask image */

		hierarchy_offset = g_data_input_stream_read_uint32 (data_stream, cancellable, error);
		if (*error != NULL)
			goto out;

		layer->alpha_mask = read_pixels_from_hierarchy (data_stream, hierarchy_offset, layer, colormap, base_type, compression, TRUE, cancellable, error);
		if (*error != NULL)
			goto out;
	}

	performance (DEBUG_INFO, "end read layers");

	surface = _cairo_image_surface_create_from_layers (canvas_width, canvas_height, base_type, layers);
	image = gth_image_new_for_surface (surface);
	cairo_surface_destroy (surface);

	performance (DEBUG_INFO, "end rendering");

out:

	g_list_free_full (layers, (GDestroyNotify) gimp_layer_free);
	if (layer_offsets != NULL)
		g_array_free (layer_offsets, TRUE);
	g_free (colormap);
	if (data_stream != NULL)
		g_object_unref (data_stream);

	return image;
}
