/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2001, 2002, 2009 The Free Software Foundation, Inc.
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

/* based upon file jpegtran.c from the libjpeg package, original copyright
 * note follows:
 *
 * jpegtran.c
 *
 * Copyright (C) 1995-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for JPEG transcoding.
 * It is very similar to cjpeg.c, but provides lossless transcoding between
 * different JPEG file formats.  It also provides some lossless and sort-of-
 * lossless transformations of JPEG data.
 */


#include <config.h>

#ifdef HAVE_LIBJPEG

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <glib.h>
#include <gthumb.h>
#include "jmemorydest.h"
#include "jmemorysrc.h"
#include "jpegtran.h"
#include "transupp.h"


GQuark
jpeg_error_quark (void)
{
	static GQuark quark;

        if (! quark)
                quark = g_quark_from_static_string ("jpeg_error");

        return quark;
}


/* error handler data */
struct error_handler_data {
	struct jpeg_error_mgr   pub;
	sigjmp_buf              setjmp_buffer;
        GError                **error;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *) cinfo->err;

        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        if ((errmgr->error != NULL) && (*errmgr->error == NULL))
                g_set_error (errmgr->error,
			     JPEG_ERROR,
			     JPEG_ERROR_FAILED,
			     "Error interpreting JPEG image\n\n%s",
                             buffer);

	siglongjmp (errmgr->setjmp_buffer, 1);
        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* void */
}


#if JPEG_LIB_VERSION < 80
static boolean
jtransform_perfect_transform (JDIMENSION  image_width,
			      JDIMENSION  image_height,
			      int         MCU_width,
			      int         MCU_height,
			      JXFORM_CODE transform)
{
	boolean result = TRUE;

	/* This function determines if it is possible to perform a lossless
	   jpeg transformation without trimming, based on the image dimensions
	   and MCU size. Further details at http://jpegclub.org/jpegtran. */

	switch (transform) {
	case JXFORM_FLIP_H:
	case JXFORM_ROT_270:
		if (image_width % (JDIMENSION) MCU_width)
			result = FALSE;
		break;
	case JXFORM_FLIP_V:
	case JXFORM_ROT_90:
		if (image_height % (JDIMENSION) MCU_height)
			result = FALSE;
		break;
	case JXFORM_TRANSVERSE:
	case JXFORM_ROT_180:
		if (image_width % (JDIMENSION) MCU_width)
			result = FALSE;
		if (image_height % (JDIMENSION) MCU_height)
			result = FALSE;
		break;
	default:
		break;
	}

	return result;
}
#endif


static gboolean
jpegtran_internal (struct jpeg_decompress_struct  *srcinfo,
		   struct jpeg_compress_struct    *dstinfo,
		   GthTransform                    transformation,
		   JCOPY_OPTION                    option,
		   JpegMcuAction                   mcu_action,
		   GError                        **error)
{
	jpeg_transform_info  transformoption;
	jvirt_barray_ptr    *src_coef_arrays;
	jvirt_barray_ptr    *dst_coef_arrays;
	JXFORM_CODE          transform;

	switch (transformation) {
	case GTH_TRANSFORM_NONE:
	default:
		transform = JXFORM_NONE;
		break;
	case GTH_TRANSFORM_FLIP_H:
		transform = JXFORM_FLIP_H;
		break;
	case GTH_TRANSFORM_FLIP_V:
		transform = JXFORM_FLIP_V;
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		transform = JXFORM_TRANSPOSE;
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		transform = JXFORM_TRANSVERSE;
		break;
	case GTH_TRANSFORM_ROTATE_90:
		transform = JXFORM_ROT_90;
		break;
	case GTH_TRANSFORM_ROTATE_180:
		transform = JXFORM_ROT_180;
		break;
	case GTH_TRANSFORM_ROTATE_270:
		transform = JXFORM_ROT_270;
		break;
	}

	transformoption.transform = transform;
	transformoption.trim = (mcu_action == JPEG_MCU_ACTION_TRIM);
	transformoption.force_grayscale = FALSE;
#if JPEG_LIB_VERSION >= 80
	transformoption.crop = 0;
#endif

	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup (srcinfo, option);

	/* Read file header */
	(void) jpeg_read_header (srcinfo, TRUE);

	/* Check JPEG Minimal Coding Unit (mcu) */
	if ((mcu_action == JPEG_MCU_ACTION_ABORT)
	    && ! jtransform_perfect_transform (srcinfo->image_width,
					       srcinfo->image_height,
					       srcinfo->max_h_samp_factor * DCTSIZE,
					       srcinfo->max_v_samp_factor * DCTSIZE,
					       transform))
	{
		if (error != NULL)
                	g_set_error (error, JPEG_ERROR, JPEG_ERROR_MCU, "MCU Error");
		return FALSE;
	}

	/* Any space needed by a transform option must be requested before
	 * jpeg_read_coefficients so that memory allocation will be done right.
	 */
	jtransform_request_workspace (srcinfo, &transformoption);

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients (srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters (srcinfo, dstinfo);

	/* Do not output a JFIF marker for EXIF thumbnails.
	 * This is not the optimal way to detect the difference
	 * between a thumbnail and a normal image, but it works
	 * well for gThumb. */
	if (option == JCOPYOPT_NONE)
		dstinfo->write_JFIF_header = FALSE;

	/* Adjust destination parameters if required by transform options;
	 * also find out which set of coefficient arrays will hold the output.
	 */
	dst_coef_arrays = jtransform_adjust_parameters (srcinfo,
							dstinfo,
							src_coef_arrays,
							&transformoption);

	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients (dstinfo, dst_coef_arrays);

	/* Copy to the output file any extra markers that we want to
	 * preserve */
	jcopy_markers_execute (srcinfo, dstinfo, option);

	/* Execute image transformation, if any */
	jtransform_execute_transformation (srcinfo,
					   dstinfo,
					   src_coef_arrays,
					   &transformoption);

	/* Finish compression */
	jpeg_finish_compress (dstinfo);
	jpeg_finish_decompress (srcinfo);

	return TRUE;
}


gboolean
jpegtran (void           *in_buffer,
	  gsize           in_buffer_size,
	  void          **out_buffer,
	  gsize          *out_buffer_size,
	  GthTransform    transformation,
	  JpegMcuAction   mcu_action,
	  GError        **error)
{
	struct jpeg_decompress_struct  srcinfo;
	struct jpeg_compress_struct    dstinfo;
	struct error_handler_data      jsrcerr, jdsterr;
	gboolean                       success;

	*out_buffer = NULL;
	*out_buffer_size = 0;

	/* Initialize the JPEG decompression object with default error
	 * handling. */
	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	/* Initialize the JPEG compression object with default error
	 * handling. */
	dstinfo.err = jpeg_std_error (&(jdsterr.pub));
	jdsterr.pub.error_exit = fatal_error_handler;
	jdsterr.pub.output_message = output_message_handler;
	jdsterr.error = error;

	jpeg_create_compress (&dstinfo);

	dstinfo.err->trace_level = 0;
	dstinfo.arith_code = FALSE;
	dstinfo.optimize_coding = FALSE;

	jsrcerr.pub.trace_level = jdsterr.pub.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

	/* Decompression error handler */
	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		return FALSE;
	}

	/* Compression error handler */
	if (sigsetjmp (jdsterr.setjmp_buffer, 1)) {
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		return FALSE;
	}

	/* Specify data source for decompression */
	_jpeg_memory_src (&srcinfo, in_buffer, in_buffer_size);

	/* Specify data destination for compression */
	_jpeg_memory_dest (&dstinfo, out_buffer, out_buffer_size);

	/* Apply transformation */
	success = jpegtran_internal (&srcinfo, &dstinfo, transformation, JCOPYOPT_ALL, mcu_action, error);

	/* Release memory */
	jpeg_destroy_compress (&dstinfo);
	jpeg_destroy_decompress (&srcinfo);

	if (success) {
		JpegTranInfo info;

		info.in_buffer = in_buffer;
		info.in_buffer_size = in_buffer_size;
		info.out_buffer = out_buffer;
		info.out_buffer_size = out_buffer_size;
		info.transformation = transformation;
		gth_hook_invoke ("jpegtran-after", &info);
	}
	else {
		g_free (*out_buffer);
		*out_buffer_size = 0;
	}

	return success;
}

#endif /* HAVE_LIBJPEG */
