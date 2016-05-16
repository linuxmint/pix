/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2011 Free Software Foundation, Inc.
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
#include "jpeg-info.h"

static guchar
_g_input_stream_read_byte (GInputStream  *stream,
			   GCancellable  *cancellable,
			   GError       **error)
{
	guchar v;
	return (g_input_stream_read (stream, &v, 1, cancellable, error) > 0) ? v : 0;
}


static guchar
_jpeg_read_segment_marker (GInputStream  *stream,
			   GCancellable  *cancellable,
			   GError       **error)
{
	guchar marker_id;

	if (_g_input_stream_read_byte (stream, cancellable, error) != 0xff)
		return 0x00;

	while ((marker_id = _g_input_stream_read_byte (stream, cancellable, error)) == 0xff)
		/* skip padding */;

	return marker_id;
}


static gboolean
_jpeg_skip_segment_data (GInputStream  *stream,
			 guchar         marker_id,
			 GCancellable  *cancellable,
			 GError       **error)
{
	if (marker_id == 0xd9)  /* EOI => end of image */
		return FALSE;
	if (marker_id == 0xda)  /* SOS => end of header */
		return FALSE;

	if ((marker_id != 0xd0)
	    && (marker_id != 0xd1)
	    && (marker_id != 0xd2)
	    && (marker_id != 0xd3)
	    && (marker_id != 0xd4)
	    && (marker_id != 0xd5)
	    && (marker_id != 0xd6)
	    && (marker_id != 0xd7)
	    && (marker_id != 0xd8)
	    && (marker_id != 0x01))
	{
		guint h, l;
		guint segment_size;

		/* skip to the next segment */

		h = _g_input_stream_read_byte (stream, cancellable, error);
		l = _g_input_stream_read_byte (stream, cancellable, error);
		segment_size = (h << 8) + l;

		if (g_input_stream_skip (stream, segment_size - 2, cancellable, error) < 0)
			return FALSE;
	}

	return TRUE;
}


static gboolean
_jpeg_skip_to_segment (GInputStream  *stream,
		       guchar         segment_id,
		       GCancellable  *cancellable,
		       GError       **error)
{
	guchar marker_id = 0x00;

	while ((marker_id = _jpeg_read_segment_marker (stream, cancellable, error)) != 0x00) {
		if (marker_id == segment_id)
			return TRUE;
		if (! _jpeg_skip_segment_data (stream, marker_id, cancellable, error))
			return FALSE;
	}

	return FALSE;
}


static GthTransform
_jpeg_exif_orientation_from_app1_segment (guchar *in_buffer,
					  gsize   app1_segment_size)
{
	int       pos;
	guint     length;
	gboolean  is_motorola;
	guchar   *exif_data;
	guint     offset, number_of_tags, tagnum;
	int       orientation;

	/* Length includes itself, so must be at least 2 */
	/* Following Exif data length must be at least 6 */

	length = app1_segment_size;
	if (length < 6)
		return 0;

	pos = 0;

	/* Read Exif head, check for "Exif" */

	if ((in_buffer[pos++] != 0x45)
	    || (in_buffer[pos++] != 0x78)
	    || (in_buffer[pos++] != 0x69)
	    || (in_buffer[pos++] != 0x66)
	    || (in_buffer[pos++] != 0)
	    || (in_buffer[pos++] != 0))
	{
		return 0;
	}

	/* Length of an IFD entry */

	if (length < 12)
		return 0;

	exif_data = in_buffer + pos;

	/* Discover byte order */

	if ((exif_data[0] == 0x49) && (exif_data[1] == 0x49))
		is_motorola = FALSE;
	else if ((exif_data[0] == 0x4D) && (exif_data[1] == 0x4D))
		is_motorola = TRUE;
	else
		return 0;

	/* Check Tag Mark */

	if (is_motorola) {
		if (exif_data[2] != 0)
			return 0;
		if (exif_data[3] != 0x2A)
			return 0;
	}
	else {
		if (exif_data[3] != 0)
			return 0;
		if (exif_data[2] != 0x2A)
			return 0;
	}

	/* Get first IFD offset (offset to IFD0) */

	if (is_motorola) {
		if (exif_data[4] != 0)
			return 0;
		if (exif_data[5] != 0)
			return 0;
		offset = exif_data[6];
		offset <<= 8;
		offset += exif_data[7];
	}
	else {
		if (exif_data[7] != 0)
			return 0;
		if (exif_data[6] != 0)
			return 0;
		offset = exif_data[5];
		offset <<= 8;
		offset += exif_data[4];
	}

	if (offset > length - 2) /* check end of data segment */
		return 0;

	/* Get the number of directory entries contained in this IFD */

	if (is_motorola) {
		number_of_tags = exif_data[offset];
		number_of_tags <<= 8;
		number_of_tags += exif_data[offset+1];
	}
	else {
		number_of_tags = exif_data[offset+1];
		number_of_tags <<= 8;
		number_of_tags += exif_data[offset];
	}
	if (number_of_tags == 0)
		return 0;

	offset += 2;

	/* Search for Orientation Tag in IFD0 */

	for (;;) {
		if (offset > length - 12) /* check end of data segment */
			return 0;

		/* Get Tag number */

		if (is_motorola) {
			tagnum = exif_data[offset];
			tagnum <<= 8;
			tagnum += exif_data[offset+1];
		}
		else {
			tagnum = exif_data[offset+1];
			tagnum <<= 8;
			tagnum += exif_data[offset];
		}

		if (tagnum == 0x0112) /* found Orientation Tag */
			break;

		if (--number_of_tags == 0)
			return 0;

		offset += 12;
	}

	/* Get the Orientation value */

	if (is_motorola) {
		if (exif_data[offset + 8] != 0)
			return 0;
		orientation = exif_data[offset + 9];
	}
	else {
		if (exif_data[offset + 9] != 0)
			return 0;
		orientation = exif_data[offset + 8];
	}

	if (orientation > 8)
		orientation = 0;

	return (GthTransform) orientation;
}


gboolean
_jpeg_get_image_info (GInputStream  *stream,
		      int           *width,
		      int           *height,
		      GthTransform  *orientation,
		      GCancellable  *cancellable,
		      GError       **error)
{
	gboolean size_read;
	guchar   marker_id;

	size_read = FALSE;

	if (orientation != NULL)
		*orientation = GTH_TRANSFORM_NONE;

	while ((marker_id = _jpeg_read_segment_marker (stream, cancellable, error)) != 0x00) {
		gboolean segment_data_consumed = FALSE;

		if ((marker_id == 0xc0) || (marker_id == 0xc2)) { /* SOF0 or SOF1 */
			guint h, l;
			/*guint size;*/

			/* size */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			/*size = (h << 8) + l;*/

			/* data precision */

			(void) _g_input_stream_read_byte (stream, cancellable, error);

			/* height */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			if (height != NULL)
				*height = (h << 8) + l;

			/* width */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			if (width != NULL)
				*width = (h << 8) + l;

			size_read = TRUE;

			/* Skip to the end of the segment. In general this is
			 * needed but in this case we exit after reading the
			 * size. */
			/* g_input_stream_skip (stream, size - 7, cancellable, error); */

			segment_data_consumed = TRUE;
		}
		else if (marker_id == 0xe1) { /* APP1 */
			guint   h, l;
			gsize   size;
			guchar *app1_segment;

			/* size */

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			size = (h << 8) + l - 2;

			app1_segment = g_new (guchar, size);
			if (g_input_stream_read (stream, app1_segment, size, cancellable, error) > 0)
				*orientation = _jpeg_exif_orientation_from_app1_segment (app1_segment, size);

			segment_data_consumed = TRUE;

			g_free (app1_segment);
		}

		/* If we have read the size we are done because the APP1
		 * segment, if present, is always the first.  */

		if (size_read)
			break;

		if (! segment_data_consumed && ! _jpeg_skip_segment_data (stream, marker_id, cancellable, error))
			break;
	}

	return size_read;
}


GthTransform
_jpeg_exif_orientation (guchar *in_buffer,
			gsize   in_buffer_size)
{
	GInputStream *stream;
	GthTransform  orientation;

	stream = g_memory_input_stream_new_from_data (in_buffer, in_buffer_size, NULL);
	orientation = _jpeg_exif_orientation_from_stream (stream, NULL, NULL);

	g_object_unref (stream);

	return orientation;
}


GthTransform
_jpeg_exif_orientation_from_stream (GInputStream  *stream,
				    GCancellable  *cancellable,
				    GError       **error)
{
	GthTransform   orientation;

	orientation = GTH_TRANSFORM_NONE;

	if (_jpeg_read_segment_marker (stream, cancellable, error) == 0xd8) {
		if (_jpeg_skip_to_segment (stream, 0xe1, cancellable, error)) {
			guint   h, l;
			guint   app1_segment_size;
			guchar *app1_segment;

			h = _g_input_stream_read_byte (stream, cancellable, error);
			l = _g_input_stream_read_byte (stream, cancellable, error);
			app1_segment_size = (h << 8) + l - 2;

			app1_segment = g_new (guchar, app1_segment_size);
			if (g_input_stream_read (stream,
						 app1_segment,
						 app1_segment_size,
						 cancellable,
						 error) > 0)
			{
				orientation = _jpeg_exif_orientation_from_app1_segment (app1_segment, app1_segment_size);
			}

			g_free (app1_segment);
		}
	}

	return orientation;
}
