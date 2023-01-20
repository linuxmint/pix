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
#include <gtk/gtk.h>
#include <gthumb.h>
#include <extensions/jpeg_utils/jpegtran.h>
#include "gth-edit-iptc-page.h"
#include "gth-metadata-provider-exiv2.h"
#include "exiv2-utils.h"


GthMetadataCategory exiv2_metadata_category[] = {
	{ "Exif::General", N_("Exif General"), 30 },
	{ "Exif::Conditions", N_("Exif Conditions"), 31 },
	{ "Exif::Structure", N_("Exif Structure"), 32 },
	{ "Exif::Thumbnail", N_("Exif Thumbnail"), 33 },
	{ "Exif::GPS", N_("Exif GPS"), 34 },
	{ "Exif::MakerNotes", N_("Exif Maker Notes"), 35 },
	{ "Exif::Versions", N_("Exif Versions"), 36 },
	{ "Exif::Other", N_("Exif Other"), 37 },
	{ "Iptc", N_("IPTC"), 38 },
	{ "Xmp::Embedded", N_("XMP Embedded"), 39 },
	{ "Xmp::Sidecar", N_("XMP Attached"), 40 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo exiv2_metadata_info[] = {

	/* Specify the type of a field to allow to create it if it's not
	 * already present in the image. */

	{ "Exif::Image::Make", NULL, "Exif::General", 1, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Image::Model", NULL, "Exif::General", 2, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Image::Software", NULL, "Exif::General", 3, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::DateTime", NULL, "Exif::General", 4, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::SubSecTime", NULL, "Exif::General", 5, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DateTimeOriginal", NULL, "Exif::General", 6, "Ascii", GTH_METADATA_ALLOW_EVERYWHERE},
	{ "Exif::Photo::SubSecTimeOriginal", NULL, "Exif::General", 7, "Ascii", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DateTimeDigitized", NULL, "Exif::General", 8, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::SubSecTimeDigitized", NULL, "Exif::General", 9, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Artist", NULL, "Exif::General", 11, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Image::Copyright", NULL, "Exif::General", 12, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::UniqueID", NULL, "Exif::General", 13, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SoundFile", NULL, "Exif::General", 14, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::ISOSpeedRatings", NULL, "Exif::Conditions", 2, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::BrightnessValue", NULL, "Exif::Conditions", 3, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::FNumber", NULL, "Exif::Conditions", 4, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::ApertureValue", NULL, "Exif::Conditions", 5, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::MaxApertureValue", NULL, "Exif::Conditions", 6, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::ExposureTime", NULL, "Exif::Conditions", 7, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::ExposureProgram", NULL, "Exif::Conditions", 8, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureIndex", NULL, "Exif::Conditions", 9, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureBiasValue", NULL, "Exif::Conditions", 10, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ExposureMode", NULL, "Exif::Conditions", 11, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ShutterSpeedValue", NULL, "Exif::Conditions", 12, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::MeteringMode", NULL, "Exif::Conditions", 13, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::LightSource", NULL, "Exif::Conditions", 14, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::WhiteBalance", NULL, "Exif::Conditions", 15, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::Flash", NULL, "Exif::Conditions", 16, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::FlashEnergy", NULL, "Exif::Conditions", 17, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectDistance", NULL, "Exif::Conditions", 18, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectDistanceRange", NULL, "Exif::Conditions", 19, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectArea", NULL, "Exif::Conditions", 20, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SubjectLocation", NULL, "Exif::Conditions", 21, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalLength", NULL, "Exif::Conditions", 22, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::FocalLengthIn35mmFilm", NULL, "Exif::Conditions", 23, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::FocalPlaneXResolution", NULL, "Exif::Conditions", 24, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalPlaneYResolution", NULL, "Exif::Conditions", 25, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FocalPlaneResolutionUnit", NULL, "Exif::Conditions", 26, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Contrast", NULL, "Exif::Conditions", 27, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Saturation", NULL, "Exif::Conditions", 28, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::Sharpness", NULL, "Exif::Conditions", 29, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SceneType", NULL, "Exif::Conditions", 30, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SceneCaptureType", NULL, "Exif::Conditions", 31, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::CustomRendered", NULL, "Exif::Conditions", 32, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DigitalZoomRatio", NULL, "Exif::Conditions", 33, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "Exif::Photo::FileSource", NULL, "Exif::Conditions", 34, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SensingMethod", NULL, "Exif::Conditions", 35, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::DeviceSettingDescription", NULL, "Exif::Conditions", 36, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::OECF", NULL, "Exif::Conditions", 37, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SpatialFrequencyResponse", NULL, "Exif::Conditions", 38, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::SpectralSensitivity", NULL, "Exif::Conditions", 39, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::GainControl", NULL, "Exif::Conditions", 40, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::CFAPattern", NULL, "Exif::Conditions", 41, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Image::ImageWidth", NULL, "Exif::Structure", 1, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ImageLength", NULL, "Exif::Structure", 2, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Orientation", NULL, "Exif::Structure", 3, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PixelXDimension", NULL, "Exif::Structure", 4, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PixelYDimension", NULL, "Exif::Structure", 5, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::XResolution", NULL, "Exif::Structure", 6, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YResolution", NULL, "Exif::Structure", 7, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ResolutionUnit", NULL, "Exif::Structure", 8, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::Compression", NULL, "Exif::Structure", 9, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::SamplesPerPixel", NULL, "Exif::Structure", 10, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::BitsPerSample", NULL, "Exif::Structure", 11, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::PlanarConfiguration", NULL, "Exif::Structure", 12, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YCbCrSubSampling", NULL, "Exif::Structure", 13, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::YCbCrPositioning", NULL, "Exif::Structure", 14, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::PhotometricInterpretation", NULL, "Exif::Structure", 15, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ComponentsConfiguration", NULL, "Exif::Structure", 16, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::CompressedBitsPerPixel", NULL, "Exif::Structure", 17, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::StripOffset", NULL, "Exif::Structure", 18, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RowsPerStrip", NULL, "Exif::Structure", 19, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::StripByteCounts", NULL, "Exif::Structure", 20, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::JPEGInterchangeFormat", NULL, "Exif::Structure", 21, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::JPEGInterchangeFormatLength", NULL, "Exif::Structure", 22, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::TransferFunction", NULL, "Exif::Structure", 23, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::WhitePoint", NULL, "Exif::Structure", 24, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::PrimaryChromaticities", NULL, "Exif::Structure", 25, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::YCbCrCoefficients", NULL, "Exif::Structure", 26, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ReferenceBlackWhite", NULL, "Exif::Structure", 27, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::ColorSpace", NULL, "Exif::Structure", 28, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::ExifVersion", NULL, "Exif::Versions", 1, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Image::ExifTag", NULL, "Exif::Versions", 2, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::FlashpixVersion", NULL, "Exif::Versions", 3, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Iop::InteroperabilityIndex", NULL, "Exif::Versions", 4, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Iop::InteroperabilityVersion", NULL, "Exif::Versions", 5, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::InteroperabilityTag", NULL, "Exif::Versions", 6, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageFileFormat", NULL, "Exif::Versions", 7, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageWidth", NULL, "Exif::Versions", 8, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Exif::Photo::RelatedImageLength", NULL, "Exif::Versions", 9, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Exif::Photo::MakerNote", NULL, "Exif::Other", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "Exif::Photo::UserComment", NULL, "Exif::Other", 1, "Ascii", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Xmp::dc::description", NULL, "Xmp::Embedded", 0, "XmpText", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Xmp::dc::title", NULL, "Xmp::Embedded", 0, "XmpText", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Xmp::iptc::Location", NULL, "Xmp::Embedded", 0, "XmpText", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Xmp::iptc::Keywords", NULL, "Xmp::Embedded", 0, "XmpBag", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Xmp::exif::DateTimeOriginal", NULL, "Xmp::Embedded", 0, "XmpText", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Xmp::xmp::Rating", NULL, "Xmp::Embedded", 0, "XmpText", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "Iptc::Application2::Headline", NULL, "Iptc", 1, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Iptc::Application2::Caption", NULL, "Iptc", 2, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Iptc::Application2::LocationName", NULL, "Iptc", 3, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "Iptc::Application2::Keywords", NULL, "Iptc", 4, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	/* the editable fields specified in the "Other" tab of the "edit metadata" dialog */

	{ "Iptc::Application2::Copyright", NULL, "Iptc", 5, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::Credit", NULL, "Iptc", 6, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::Byline", NULL, "Iptc", 7, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::BylineTitle", NULL, "Iptc", 8, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::CountryName", NULL, "Iptc", 9, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::CountryCode", NULL, "Iptc", 10, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::City", NULL, "Iptc", 11, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::Language", NULL, "Iptc", 12, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::ObjectName", NULL, "Iptc", 13, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::Source", NULL, "Iptc", 14, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Envelope::Destination", NULL, "Iptc", 15, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },
	{ "Iptc::Application2::Urgency", NULL, "Iptc", 16, "String", GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW | GTH_METADATA_ALLOW_IN_PRINT },

	{ NULL, NULL, NULL, 0, NULL, 0 }
};


static void
update_exif_dimensions (GFileInfo    *info,
		        GthTransform  transform)
{
	g_return_if_fail (info != NULL);

	if ((transform == GTH_TRANSFORM_ROTATE_90)
	    || (transform == GTH_TRANSFORM_ROTATE_270)
	    || (transform == GTH_TRANSFORM_TRANSPOSE)
	    || (transform == GTH_TRANSFORM_TRANSVERSE))
	{
		_g_file_info_swap_attributes (info, "Exif::Photo::PixelXDimension", "Exif::Photo::PixelYDimension");
		_g_file_info_swap_attributes (info, "Exif::Image::XResolution", "Exif::Image::YResolution");
		_g_file_info_swap_attributes (info, "Exif::Photo::FocalPlaneXResolution", "Exif::Photo::FocalPlaneYResolution");
		_g_file_info_swap_attributes (info, "Exif::Image::ImageWidth", "Exif::Image::ImageLength");
		_g_file_info_swap_attributes (info, "Exif::Iop::RelatedImageWidth", "Exif::Iop::RelatedImageLength");
	}
}


static void
exiv2_jpeg_tran_cb (JpegTranInfo *tran_info)
{
	GFileInfo *info;

	info = g_file_info_new ();
	if (exiv2_read_metadata_from_buffer (tran_info->in_buffer,
					     tran_info->in_buffer_size,
					     info,
					     FALSE,
					     NULL))
	{
		GthMetadata *metadata;

		update_exif_dimensions (info, tran_info->transformation);

		metadata = g_object_new (GTH_TYPE_METADATA, "raw", "1", NULL);
		g_file_info_set_attribute_object (info, "Exif::Image::Orientation", G_OBJECT (metadata));
		exiv2_write_metadata_to_buffer (tran_info->out_buffer, tran_info->out_buffer_size, info, NULL, NULL);

		g_object_unref (metadata);
	}

	g_object_unref (info);
}


static void
exiv2_delete_metadata_cb (GFile  *file,
			  void  **buffer,
			  gsize  *size)
{
	exiv2_clear_metadata (buffer, size, NULL);
}


static int
gth_file_data_cmp_date_time_original (GthFileData *a,
				      GthFileData *b)
{
	GTimeVal *pta, *ptb;
	GTimeVal  ta, tb;
	int       result;

	pta = NULL;
	if (gth_file_data_get_digitalization_time (a, &ta))
		pta = &ta;
	if (pta == NULL)
		pta = gth_file_data_get_modification_time (a);

	ptb = NULL;
	if (gth_file_data_get_digitalization_time (b, &tb))
		ptb = &tb;
	if (ptb == NULL)
		ptb = gth_file_data_get_modification_time (b);

	result = _g_time_val_cmp (pta, ptb);
	if (result == 0) {
		const char *key_a, *key_b;

		key_a = gth_file_data_get_filename_sort_key (a);
		key_b = gth_file_data_get_filename_sort_key (b);

		result = strcmp (key_a, key_b);
	}

	return result;
}


GthFileDataSort exiv2_sort_types[] = {
	{ "exif::photo::datetimeoriginal", N_("date photo was taken"),
	  "Exif::Photo::DateTimeOriginal,Exif::Photo::DateTimeDigitized",
	  gth_file_data_cmp_date_time_original }
};


static void
exiv2_add_sidecars_cb (GFile  *file,
		       GList **sidecars)
{
	*sidecars = g_list_prepend (*sidecars, exiv2_get_sidecar (file));
}


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	int i;

	gth_main_register_metadata_category (exiv2_metadata_category);
	gth_main_register_metadata_info_v (exiv2_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_EXIV2);
	if (gth_main_extension_is_active ("edit_metadata")) {
		gth_main_register_type ("edit-comment-dialog-page", GTH_TYPE_EDIT_IPTC_PAGE);
		gth_hook_add_callback ("delete-metadata", 10, G_CALLBACK (exiv2_delete_metadata_cb), NULL);
	}
	gth_hook_add_callback ("save-image", 10, G_CALLBACK (exiv2_write_metadata), NULL);
	if (gth_hook_present ("jpegtran-after"))
		gth_hook_add_callback ("jpegtran-after", 10, G_CALLBACK (exiv2_jpeg_tran_cb), NULL);
	gth_hook_add_callback ("generate-thumbnail", 10, G_CALLBACK (exiv2_generate_thumbnail), NULL);
	gth_hook_add_callback ("add-sidecars", 10, G_CALLBACK (exiv2_add_sidecars_cb), NULL);

	for (i = 0; i < G_N_ELEMENTS (exiv2_sort_types); i++)
		gth_main_register_sort_type (&exiv2_sort_types[i]);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
