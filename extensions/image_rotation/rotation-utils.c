/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2007, 2009 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <sys/types.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#ifdef HAVE_LIBJPEG
#include <extensions/jpeg_utils/jpegtran.h>
#endif
#include "rotation-utils.h"


enum {
	GTH_RESPONSE_TRIM
};


typedef struct {
	GtkWidget        *dialog;
	TrimResponseFunc  done_func;
	gpointer          done_data;
} AskTrimData;


static void
ask_whether_to_trim_response_cb (GtkDialog *dialog,
                                 gint       response,
                                 gpointer   user_data)
{
	AskTrimData *data = user_data;

 	gtk_widget_destroy (data->dialog);

	if (data->done_func != NULL) {
		JpegMcuAction action;

		switch (response) {
		case GTH_RESPONSE_TRIM:
			action = JPEG_MCU_ACTION_TRIM;
			break;
		case GTK_RESPONSE_OK:
			action = JPEG_MCU_ACTION_DONT_TRIM;
			break;
		default:
			action = JPEG_MCU_ACTION_ABORT;
			break;
		}
		data->done_func (action, data->done_data);
	}

	g_free (data);
}


GtkWidget *
ask_whether_to_trim (GtkWindow        *parent_window,
		     GthFileData      *file_data,
		     TrimResponseFunc  done_func,
		     gpointer          done_data)
{
	AskTrimData *data;
	char        *filename;
	char        *msg;

	/* If the user disabled the warning dialog trim the image */

	/* FIXME
	 if (! eel_gconf_get_boolean (PREF_MSG_JPEG_MCU_WARNING, TRUE)) {
		if (done_func != NULL)
			done_func (JPEG_MCU_ACTION_TRIM, done_data);
		return;
	}
	*/

	/*
	 * Image dimensions are not multiples of the jpeg minimal coding unit (mcu).
	 * Warn about possible image distortions along one or more edges.
	 */

	data = g_new0 (AskTrimData, 1);
	data->done_func = done_func;
	data->done_data = done_data;

	filename = g_file_get_parse_name (file_data->file);
	msg = g_strdup_printf (_("Problem transforming the image: %s"), filename);
	data->dialog = _gtk_message_dialog_new (parent_window,
						GTK_DIALOG_MODAL,
						_GTK_ICON_NAME_DIALOG_WARNING,
						msg,
						_("This transformation may introduce small image distortions along "
                                                  "one or more edges, because the image dimensions are not multiples of 8.\n\nThe distortion "
						  "is reversible, however. If the resulting image is unacceptable, simply apply the reverse "
						  "transformation to return to the original image.\n\nYou can also choose to discard (or trim) any "
						  "untransformable edge pixels. For practical use, this mode gives the best looking results, "
						  "but the transformation is not strictly lossless anymore."),
						_("_Trim"), GTH_RESPONSE_TRIM,
					        _GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
					        _("_Accept distortion"), GTK_RESPONSE_OK,
					        NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (data->dialog),
			  "response",
			  G_CALLBACK (ask_whether_to_trim_response_cb),
			  data);
	gtk_widget_show (data->dialog);

	g_free (msg);
	g_free (filename);

	return data->dialog;
}


/* -- get_next_transformation -- */


static GthTransform
get_next_value_rotation_90 (GthTransform value)
{
	static GthTransform new_value [8] = {6, 7, 8, 5, 2, 3, 4, 1};
	return new_value[value - 1];
}


static GthTransform
get_next_value_mirror (GthTransform value)
{
	static GthTransform new_value [8] = {2, 1, 4, 3, 6, 5, 8, 7};
	return new_value[value - 1];
}


static GthTransform
get_next_value_flip (GthTransform value)
{
	static GthTransform new_value [8] = {4, 3, 2, 1, 8, 7, 6, 5};
	return new_value[value - 1];
}


GthTransform
get_next_transformation (GthTransform original,
			 GthTransform transform)
{
	GthTransform result;

	result = ((original >= 1) && (original <= 8)) ? original : GTH_TRANSFORM_NONE;
	switch (transform) {
	case GTH_TRANSFORM_NONE:
		break;
	case GTH_TRANSFORM_ROTATE_90:
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_ROTATE_180:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_ROTATE_270:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		result = get_next_value_rotation_90 (result);
		break;
	case GTH_TRANSFORM_FLIP_H:
		result = get_next_value_mirror (result);
		break;
	case GTH_TRANSFORM_FLIP_V:
		result = get_next_value_flip (result);
		break;
	case GTH_TRANSFORM_TRANSPOSE:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_mirror (result);
		break;
	case GTH_TRANSFORM_TRANSVERSE:
		result = get_next_value_rotation_90 (result);
		result = get_next_value_flip (result);
		break;
	}

	return result;
}


/* -- apply_transformation_async -- */


typedef struct {
	GthFileData   *file_data;
	GthTransform   transform;
	JpegMcuAction  mcu_action;
	GCancellable  *cancellable;
	ReadyFunc      ready_func;
	gpointer       user_data;
} TransformatioData;


static void
transformation_data_free (TransformatioData *tdata)
{
	_g_object_unref (tdata->file_data);
	_g_object_unref (tdata->cancellable);
	g_free (tdata);
}


#ifdef HAVE_LIBJPEG
static void
write_file_ready_cb (void     **buffer,
		     gsize      count,
		     GError    *error,
		     gpointer   user_data)
{
	TransformatioData *tdata = user_data;

	tdata->ready_func (error, tdata->user_data);
	transformation_data_free (tdata);
}
#endif /* HAVE_LIBJPEG */


static void
pixbuf_saved_cb (GthFileData *file_data,
		 GError      *error,
		 gpointer     user_data)
{
	TransformatioData *tdata = user_data;

	tdata->ready_func (error, tdata->user_data);
	transformation_data_free (tdata);
}


static void
file_buffer_ready_cb (void     **buffer,
		      gsize      count,
		      GError    *error,
		      gpointer   user_data)
{
	TransformatioData *tdata = user_data;
	GthMetadata       *metadata;
	GthTransform       orientation;

	if (error != NULL) {
		tdata->ready_func (error, tdata->user_data);
		transformation_data_free (tdata);
		return;
	}

	orientation = GTH_TRANSFORM_NONE;
	metadata = (GthMetadata *) g_file_info_get_attribute_object (tdata->file_data->info, "Embedded::Image::Orientation");
	if ((metadata != NULL) && (gth_metadata_get_raw (metadata) != NULL))
		orientation = strtol (gth_metadata_get_raw (metadata), (char **) NULL, 10);
	orientation = get_next_transformation (orientation, tdata->transform);

#ifdef HAVE_LIBJPEG
	if (g_content_type_equals (gth_file_data_get_mime_type (tdata->file_data), "image/jpeg")) {
		void  *out_buffer;
		gsize  out_buffer_size;

		if (! jpegtran (*buffer,
				count,
				&out_buffer,
				&out_buffer_size,
				orientation,
				tdata->mcu_action,
				&error))
		{
			tdata->ready_func (error, tdata->user_data);
			transformation_data_free (tdata);
			return;
		}

		_g_file_write_async (tdata->file_data->file,
				     out_buffer,
		    		     out_buffer_size,
		    		     TRUE,
				     G_PRIORITY_DEFAULT,
				     tdata->cancellable,
				     write_file_ready_cb,
				     tdata);
	}
	else
#endif /* HAVE_LIBJPEG */
	{
		GInputStream    *istream;
		GthImage        *image;
		cairo_surface_t *surface;
		cairo_surface_t *transformed;

		istream = g_memory_input_stream_new_from_data (*buffer, count, NULL);
		image = gth_image_new_from_stream (istream, -1, NULL, NULL, tdata->cancellable, &error);
		if (image == NULL) {
			tdata->ready_func (error, tdata->user_data);
			transformation_data_free (tdata);
			return;
		}

		surface = gth_image_get_cairo_surface (image);
		transformed = _cairo_image_surface_transform (surface, orientation);
		gth_image_set_cairo_surface (image, transformed);
		gth_image_save_to_file (image,
					gth_file_data_get_mime_type (tdata->file_data),
					tdata->file_data,
					TRUE,
					tdata->cancellable,
					pixbuf_saved_cb,
					tdata);

		cairo_surface_destroy (transformed);
		cairo_surface_destroy (surface);
		g_object_unref (image);
		g_object_unref (istream);
	}
}


void
apply_transformation_async (GthFileData   *file_data,
			    GthTransform   transform,
			    JpegMcuAction  mcu_action,
			    GCancellable  *cancellable,
			    ReadyFunc      ready_func,
			    gpointer       user_data)
{
	TransformatioData *tdata;

	tdata = g_new0 (TransformatioData, 1);
	tdata->file_data = g_object_ref (file_data);
	tdata->transform = transform;
	tdata->mcu_action = mcu_action;
	tdata->cancellable = _g_object_ref (cancellable);
	tdata->ready_func = ready_func;
	tdata->user_data = user_data;

	_g_file_load_async (tdata->file_data->file,
			    G_PRIORITY_DEFAULT,
			    tdata->cancellable,
			    file_buffer_ready_cb,
			    tdata);
}
