/* -*- Mode: CPP; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008-2009 Free Software Foundation, Inc.
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib.h>
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <exiv2/basicio.hpp>
#include <exiv2/error.hpp>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <exiv2/exiv2.hpp>
#if EXIV2_TEST_VERSION(0, 27, 0)
#include <exiv2/xmp_exiv2.hpp>
#else
#include <exiv2/xmp.hpp>
#endif
#include <gthumb.h>
#include "exiv2-utils.h"

using namespace std;


#define INVALID_VALUE N_("(invalid value)")


/* Some bits of information may be contained in more than one metadata tag.
   The arrays below define the valid tags for a particular piece of
   information, in decreasing order of preference (best one first) */

const char *_DATE_TAG_NAMES[] = {
	"Exif::Image::DateTime",
	"Xmp::exif::DateTime",
	"Exif::Photo::DateTimeOriginal",
	"Xmp::exif::DateTimeOriginal",
	"Exif::Photo::DateTimeDigitized",
	"Xmp::exif::DateTimeDigitized",
	"Xmp::xmp::CreateDate",
	"Xmp::photoshop::DateCreated",
	"Xmp::xmp::ModifyDate",
	"Xmp::xmp::MetadataDate",
	NULL
};

const char *_LAST_DATE_TAG_NAMES[] = {
	"Exif::Image::DateTime",
	"Xmp::exif::DateTime",
	"Xmp::xmp::ModifyDate",
	"Xmp::xmp::MetadataDate",
	NULL
};

const char *_ORIGINAL_DATE_TAG_NAMES[] = {
	"Exif::Photo::DateTimeOriginal",
	"Xmp::exif::DateTimeOriginal",
	"Exif::Photo::DateTimeDigitized",
	"Xmp::exif::DateTimeDigitized",
	"Xmp::xmp::CreateDate",
	"Xmp::photoshop::DateCreated",
	NULL
};

const char *_EXPOSURE_TIME_TAG_NAMES[] = {
	"Exif::Photo::ExposureTime",
	"Xmp::exif::ExposureTime",
	"Exif::Photo::ShutterSpeedValue",
	"Xmp::exif::ShutterSpeedValue",
	NULL
};

const char *_EXPOSURE_MODE_TAG_NAMES[] = {
	"Exif::Photo::ExposureMode",
	"Xmp::exif::ExposureMode",
	NULL
};

const char *_ISOSPEED_TAG_NAMES[] = {
	"Exif::Photo::ISOSpeedRatings",
	"Xmp::exif::ISOSpeedRatings",
	NULL
};

const char *_APERTURE_TAG_NAMES[] = {
	"Exif::Photo::ApertureValue",
	"Xmp::exif::ApertureValue",
	"Exif::Photo::FNumber",
	"Xmp::exif::FNumber",
	NULL
};

const char *_FOCAL_LENGTH_TAG_NAMES[] = {
	"Exif::Photo::FocalLength",
	"Xmp::exif::FocalLength",
	NULL
};

const char *_SHUTTER_SPEED_TAG_NAMES[] = {
	"Exif::Photo::ShutterSpeedValue",
	"Xmp::exif::ShutterSpeedValue",
	NULL
};

const char *_MAKE_TAG_NAMES[] = {
	"Exif::Image::Make",
	"Xmp::tiff::Make",
	NULL
};

const char *_MODEL_TAG_NAMES[] = {
	"Exif::Image::Model",
	"Xmp::tiff::Model",
	NULL
};

const char *_FLASH_TAG_NAMES[] = {
	"Exif::Photo::Flash",
	"Xmp::exif::Flash",
	NULL
};

const char *_ORIENTATION_TAG_NAMES[] = {
	"Exif::Image::Orientation",
	"Xmp::tiff::Orientation",
	NULL
};

const char *_DESCRIPTION_TAG_NAMES[] = {
	"Iptc::Application2::Caption",
	"Xmp::dc::description",
	"Exif::Photo::UserComment",
	"Exif::Image::ImageDescription",
	"Xmp::tiff::ImageDescription",
	"Iptc::Application2::Headline",
	NULL
};

const char *_TITLE_TAG_NAMES[] = {
	"Xmp::dc::title",
	NULL
};

const char *_LOCATION_TAG_NAMES[] = {
	"Xmp::iptc::Location",
	"Iptc::Application2::LocationName",
	NULL
};

const char *_KEYWORDS_TAG_NAMES[] = {
	"Iptc::Application2::Keywords",
	"Xmp::iptc::Keywords",
	"Xmp::dc::subject",
	NULL
};

const char *_RATING_TAG_NAMES[] = {
	"Xmp::xmp::Rating",
	NULL
};

const char *_AUTHOR_TAG_NAMES[] = {
	"Exif::Image::Artist",
	"Iptc::Application2::Byline",
	"Xmp::dc::creator",
	"Xmp::xmpDM::artist",
	"Xmp::tiff::Artist",
	"Xmp::plus::ImageCreator",
	"Xmp::plus::ImageCreatorName",
	NULL
};

const char *_COPYRIGHT_TAG_NAMES[] = {
	"Exif::Image::Copyright",
	"Iptc::Application2::Copyright",
	"Xmp::dc::rights",
	"Xmp::xmpDM::copyright",
	"Xmp::tiff::Copyright",
	"Xmp::plus::CopyrightOwner",
	"Xmp::plus::CopyrightOwnerName",
	NULL
};

/* Some cameras fill in the ImageDescription or UserComment fields
   with useless fluff. Try to filter these out, so they do not show up
   as comments. */
const char *useless_comment_filter[] = {
	"OLYMPUS DIGITAL CAMERA",
	"SONY DSC",
	"KONICA MINOLTA DIGITAL CAMERA",
	"MINOLTA DIGITAL CAMERA",
	"binary comment",
	NULL
};


inline static char *
exiv2_key_from_attribute (const char *attribute)
{
	return _g_utf8_replace_str (attribute, "::", ".");
}


inline static char *
exiv2_key_to_attribute (const char *key)
{
	return _g_utf8_replace_str (key, ".", "::");
}


static gboolean
attribute_is_date (const char *key)
{
	int i;

	for (i = 0; _DATE_TAG_NAMES[i] != NULL; i++) {
		if (strcmp (_DATE_TAG_NAMES[i], key) == 0)
			return TRUE;
	}

	return FALSE;
}


static GthMetadata *
create_metadata (const char *key,
		 const char *description,
		 const char *formatted_value,
		 const char *raw_value,
		 const char *category,
		 const char *type_name)
{
	char            *formatted_value_utf8;
	char            *attribute;
	GthMetadataInfo *metadata_info;
	GthMetadata     *metadata;
	char            *description_utf8;

	formatted_value_utf8 = _g_utf8_from_any (formatted_value);
	if (_g_utf8_all_spaces (formatted_value_utf8))
		return NULL;

	description_utf8 = _g_utf8_from_any (description);

	attribute = exiv2_key_to_attribute (key);
	if (attribute_is_date (attribute)) {
		GTimeVal time_;

		g_free (formatted_value_utf8);
		formatted_value_utf8 = NULL;

		if (_g_time_val_from_exif_date (raw_value, &time_))
			formatted_value_utf8 = _g_time_val_strftime (&time_, "%x %X");
		else
			formatted_value_utf8 = g_locale_to_utf8 (formatted_value, -1, NULL, NULL, NULL);
	}
	else {
		char *tmp = _g_utf8_remove_string_properties (formatted_value_utf8);
		g_free (formatted_value_utf8);
		formatted_value_utf8 = tmp;
	}

	if (formatted_value_utf8 == NULL)
		formatted_value_utf8 = g_strdup (INVALID_VALUE);

	metadata_info = gth_main_get_metadata_info (attribute);
	if ((metadata_info == NULL) && (category != NULL)) {
		GthMetadataInfo info;

		info.id = attribute;
		info.type = (type_name != NULL) ? g_strdup (type_name) : NULL;
		info.display_name = description_utf8;
		info.category = category;
		info.sort_order = 500;
		info.flags = GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW;
		metadata_info = gth_main_register_metadata_info (&info);
	}

	if ((metadata_info != NULL) && (metadata_info->type == NULL) && (type_name != NULL))
		metadata_info->type = g_strdup (type_name);

	if ((metadata_info != NULL) && (metadata_info->display_name == NULL) && (description_utf8 != NULL))
		metadata_info->display_name = g_strdup (description_utf8);

	metadata = gth_metadata_new ();
	g_object_set (metadata,
		      "id", key,
		      "description", description_utf8,
		      "formatted", formatted_value_utf8,
		      "raw", raw_value,
		      "value-type", type_name,
		      NULL);

	g_free (formatted_value_utf8);
	g_free (description_utf8);
	g_free (attribute);

	return metadata;
}


static void
add_string_list_to_metadata (GthMetadata            *metadata,
			     const Exiv2::Metadatum &value)
{
	GList         *list;
	GthStringList *string_list;

	list = NULL;
	for (int i = 0; i < value.count(); i++)
		list = g_list_prepend (list, g_strdup (value.toString(i).c_str()));
	string_list = gth_string_list_new (g_list_reverse (list));
	g_object_set (metadata, "string-list", string_list, NULL);

	g_object_unref (string_list);
	_g_string_list_free (list);
}


static void
set_file_info (GFileInfo  *info,
	       const char *key,
	       const char *description,
	       const char *formatted_value,
	       const char *raw_value,
	       const char *category,
	       const char *type_name)
{
	char        *attribute;
	GthMetadata *metadata;

	attribute = exiv2_key_to_attribute (key);
	metadata = create_metadata (key, description, formatted_value, raw_value, category, type_name);
	if (metadata != NULL) {
		g_file_info_set_attribute_object (info, attribute, G_OBJECT (metadata));
		g_object_unref (metadata);
	}

	g_free (attribute);
}


GHashTable *
create_metadata_hash (void)
{
	return g_hash_table_new_full (g_str_hash,
				      g_str_equal,
				      g_free,
				      g_object_unref);
}


static void
add_metadata_to_hash (GHashTable  *table,
		      GthMetadata *metadata)
{
	char     *key;
	gpointer  object;

	if (metadata == NULL)
		return;

	key = exiv2_key_to_attribute (gth_metadata_get_id (metadata));
	object = g_hash_table_lookup (table, key);
	if (object != NULL) {
		GthStringList *string_list;
		GList         *list;

		string_list = NULL;
		switch (gth_metadata_get_data_type (GTH_METADATA (object))) {
		case GTH_METADATA_TYPE_STRING:
			string_list = gth_string_list_new (NULL);
			list = g_list_append (NULL, g_strdup (gth_metadata_get_formatted (GTH_METADATA (object))));
			gth_string_list_set_list (string_list, list);
			break;

		case GTH_METADATA_TYPE_STRING_LIST:
			string_list = (GthStringList *) g_object_ref (gth_metadata_get_string_list (GTH_METADATA (object)));
			break;
		}

		if (string_list == NULL) {
			g_hash_table_insert (table,
					     g_strdup (key),
					     g_object_ref (metadata));
			return;
		}

		switch (gth_metadata_get_data_type (metadata)) {
		case GTH_METADATA_TYPE_STRING:
			list = gth_string_list_get_list (string_list);
			list = g_list_append (list, g_strdup (gth_metadata_get_formatted (metadata)));
			gth_string_list_set_list (string_list, list);
			break;

		case GTH_METADATA_TYPE_STRING_LIST:
			gth_string_list_concat (string_list, gth_metadata_get_string_list (metadata));
			break;
		}

		g_object_set (metadata, "string-list", string_list, NULL);
		g_hash_table_replace (table,
				      g_strdup (key),
				      g_object_ref (metadata));

		g_object_unref (string_list);
	}
	else
		g_hash_table_insert (table,
				     g_strdup (key),
				     g_object_ref (metadata));

	g_free (key);
}


static void
set_file_info_from_hash (GFileInfo  *info,
			 GHashTable *table)
{
	GHashTableIter iter;
	gpointer       key;
	gpointer       value;

	g_hash_table_iter_init (&iter, table);
	while (g_hash_table_iter_next (&iter, &key, &value))
		g_file_info_set_attribute_object (info, (char *)key, G_OBJECT (value));
}


static void
set_attribute_from_metadata (GFileInfo  *info,
			     const char *attribute,
			     GObject    *metadata)
{
	char *description;
	char *formatted_value;
	char *raw_value;
	char *type_name;

	if (metadata == NULL)
		return;

	g_object_get (metadata,
		      "description", &description,
		      "formatted", &formatted_value,
		      "raw", &raw_value,
		      "value-type", &type_name,
		      NULL);

	set_file_info (info,
		       attribute,
		       description,
		       formatted_value,
		       raw_value,
		       NULL,
		       type_name);

	g_free (description);
	g_free (formatted_value);
	g_free (raw_value);
	g_free (type_name);
}


static GObject *
get_attribute_from_tagset (GFileInfo  *info,
			   const char *tagset[])
{
	GObject *metadata;
	int      i;

	metadata = NULL;
	for (i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			return metadata;
	}

	return NULL;
}


static void
set_attribute_from_tagset (GFileInfo  *info,
			   const char *attribute,
			   const char *tagset[])
{
	GObject *metadata;

	metadata = get_attribute_from_tagset (info, tagset);
	if (metadata != NULL)
		set_attribute_from_metadata (info, attribute, metadata);
}


static void
set_string_list_attribute_from_tagset (GFileInfo  *info,
				       const char *attribute,
				       const char *tagset[])
{
	GObject *metadata;
	int      i;

	metadata = NULL;
	for (i = 0; tagset[i] != NULL; i++) {
		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if (metadata != NULL)
			break;
	}

	if (metadata == NULL)
		return;

	if (GTH_IS_METADATA (metadata) && (gth_metadata_get_data_type (GTH_METADATA (metadata)) != GTH_METADATA_TYPE_STRING_LIST)) {
		char           *raw;
		char           *utf8_raw;
		char          **keywords;
		GthStringList  *string_list;

		g_object_get (metadata, "raw", &raw, NULL);
		utf8_raw = _g_utf8_try_from_any (raw);
		if (utf8_raw == NULL)
			return;

		keywords = g_strsplit (utf8_raw, ",", -1);
		string_list = gth_string_list_new_from_strv (keywords);
		metadata = (GObject *) gth_metadata_new_for_string_list (string_list);
		g_file_info_set_attribute_object (info, attribute, metadata);

		g_object_unref (metadata);
		g_object_unref (string_list);
		g_strfreev (keywords);
		g_free (raw);
		g_free (utf8_raw);
	}
	else
		g_file_info_set_attribute_object (info, attribute, metadata);
}


static void
clear_useless_comments_from_tagset (GFileInfo  *info,
				   const char *tagset[])
{
	int i;

	for (i = 0; tagset[i] != NULL; i++) {
		GObject    *metadata;
		const char *value;
		int         j;

		metadata = g_file_info_get_attribute_object (info, tagset[i]);
		if ((metadata == NULL) || ! GTH_IS_METADATA (metadata))
			continue;

		value = gth_metadata_get_formatted (GTH_METADATA (metadata));
		for (j = 0; useless_comment_filter[j] != NULL; j++) {
			if (strstr (value, useless_comment_filter[j]) == value) {
				g_file_info_remove_attribute (info, tagset[i]);
				break;
			}
		}
	}
}


extern "C"
void
exiv2_update_general_attributes (GFileInfo *info)
{
	set_attribute_from_tagset (info, "general::datetime", _ORIGINAL_DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "general::description", _DESCRIPTION_TAG_NAMES);
	set_attribute_from_tagset (info, "general::title", _TITLE_TAG_NAMES);

	/* if iptc::caption and iptc::headline are different use iptc::headline
	 * to set general::title, if not already set. */

	if (g_file_info_get_attribute_object (info, "general::title") == NULL) {
		GObject *iptc_caption;
		GObject *iptc_headline;

		iptc_caption = g_file_info_get_attribute_object (info, "Iptc::Application2::Caption");
		iptc_headline = g_file_info_get_attribute_object (info, "Iptc::Application2::Headline");

		if ((iptc_caption != NULL)
		    && (iptc_headline != NULL)
		    && (g_strcmp0 (gth_metadata_get_raw (GTH_METADATA (iptc_caption)),
				   gth_metadata_get_raw (GTH_METADATA (iptc_headline))) != 0))
		{
			set_attribute_from_metadata (info, "general::title", iptc_headline);
		}
	}

	set_attribute_from_tagset (info, "general::location", _LOCATION_TAG_NAMES);
	set_string_list_attribute_from_tagset (info, "general::tags", _KEYWORDS_TAG_NAMES);
	set_attribute_from_tagset (info, "general::rating", _RATING_TAG_NAMES);
}


#define EXPOSURE_SEPARATOR " · "


static void
set_attributes_from_tagsets (GFileInfo *info,
			     gboolean   update_general_attributes)
{
	clear_useless_comments_from_tagset (info, _DESCRIPTION_TAG_NAMES);
	clear_useless_comments_from_tagset (info, _TITLE_TAG_NAMES);

	if (update_general_attributes)
		exiv2_update_general_attributes (info);

	set_attribute_from_tagset (info, "Embedded::Photo::DateTimeOriginal", _ORIGINAL_DATE_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Image::Orientation", _ORIENTATION_TAG_NAMES);

	set_attribute_from_tagset (info, "Embedded::Photo::Aperture", _APERTURE_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::ISOSpeed", _ISOSPEED_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::ExposureTime", _EXPOSURE_TIME_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::ShutterSpeed", _SHUTTER_SPEED_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::FocalLength", _FOCAL_LENGTH_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::Flash", _FLASH_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::CameraModel", _MODEL_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::Author", _AUTHOR_TAG_NAMES);
	set_attribute_from_tagset (info, "Embedded::Photo::Copyright", _COPYRIGHT_TAG_NAMES);

	/* Embedded::Photo::Exposure */

	GObject *aperture;
	GObject *iso_speed;
	GObject *shutter_speed;
	GObject *exposure_time;
	GString *exposure;

	aperture = get_attribute_from_tagset (info, _APERTURE_TAG_NAMES);
	iso_speed = get_attribute_from_tagset (info, _ISOSPEED_TAG_NAMES);
	shutter_speed = get_attribute_from_tagset (info, _SHUTTER_SPEED_TAG_NAMES);
	exposure_time = get_attribute_from_tagset (info, _EXPOSURE_TIME_TAG_NAMES);

	exposure = g_string_new ("");

	if (aperture != NULL) {
		char *formatted_value;

		g_object_get (aperture, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	if (iso_speed != NULL) {
		char *formatted_value;

		g_object_get (iso_speed, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, "ISO ");
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	if (shutter_speed != NULL) {
		char *formatted_value;

		g_object_get (shutter_speed, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}
	else if (exposure_time != NULL) {
		char *formatted_value;

		g_object_get (exposure_time, "formatted", &formatted_value, NULL);
		if (formatted_value != NULL) {
			if (exposure->len > 0)
				g_string_append (exposure, EXPOSURE_SEPARATOR);
			g_string_append (exposure, formatted_value);
			g_free (formatted_value);
		}
	}

	set_file_info (info,
		       "Embedded::Photo::Exposure",
		       _("Exposure"),
		       exposure->str,
		       NULL,
		       NULL,
		       NULL);

	g_string_free (exposure, TRUE);
}


static const char *
get_exif_default_category (const Exiv2::Exifdatum &md)
{
#if EXIV2_TEST_VERSION(0, 21, 0)
	if (Exiv2::ExifTags::isMakerGroup(md.groupName()))
#else
	if (Exiv2::ExifTags::isMakerIfd(md.ifdId()))
#endif
		return "Exif::MakerNotes";

	if (md.groupName().compare("Thumbnail") == 0)
		return "Exif::Thumbnail";
	else if (md.groupName().compare("GPSInfo") == 0)
		return "Exif::GPS";
	else if (md.groupName().compare("Iop") == 0)
		return "Exif::Versions";

	return "Exif::Other";
}


static void
exiv2_read_metadata (Exiv2::Image::AutoPtr  image,
		     GFileInfo             *info,
		     gboolean               update_general_attributes)
{
	image->readMetadata();

	Exiv2::ExifData &exifData = image->exifData();
	if (! exifData.empty()) {
		Exiv2::ExifData::const_iterator end = exifData.end();
		for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
#if EXIV2_TEST_VERSION(0, 21, 0)
			else if (Exiv2::ExifTags::isMakerGroup(md->groupName()))
#else
			else if (Exiv2::ExifTags::isMakerIfd(md->ifdId()))
#endif
				// Must be a MakerNote - include group name
				description << md->groupName() << "." << md->tagName();
			else
				// Normal exif tag - just use tag name
				description << md->tagName();

			set_file_info (info,
				       md->key().c_str(),
				       description.str().c_str(),
				       md->print(&exifData).c_str(),
				       raw_value.str().c_str(),
				       get_exif_default_category (*md),
				       md->typeName());
		}
	}

	Exiv2::IptcData &iptcData = image->iptcData();
	if (! iptcData.empty()) {
		GHashTable *table = create_metadata_hash ();

		Exiv2::IptcData::iterator end = iptcData.end();
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->tagName();

			GthMetadata *metadata;
			metadata = create_metadata (md->key().c_str(),
						    description.str().c_str(),
						    md->print().c_str(),
						    raw_value.str().c_str(),
						    "Iptc",
						    md->typeName());
			if (metadata != NULL) {
				add_metadata_to_hash (table, metadata);
				g_object_unref (metadata);
			}
		}

		set_file_info_from_hash (info, table);
		g_hash_table_unref (table);
	}

	Exiv2::XmpData &xmpData = image->xmpData();
	if (! xmpData.empty()) {
		GHashTable *table = create_metadata_hash ();

		Exiv2::XmpData::iterator end = xmpData.end();
		for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
			stringstream raw_value;
			raw_value << md->value();

			stringstream description;
			if (! md->tagLabel().empty())
				description << md->tagLabel();
			else
				description << md->groupName() << "." << md->tagName();

			GthMetadata *metadata;
			metadata = create_metadata (md->key().c_str(),
						    description.str().c_str(),
						    md->print().c_str(),
						    raw_value.str().c_str(),
						    "Xmp::Embedded",
						    md->typeName());
			if (metadata != NULL) {
				if ((g_strcmp0 (md->typeName(), "XmpBag") == 0)
				    || (g_strcmp0 (md->typeName(), "XmpSeq") == 0))
				{
					add_string_list_to_metadata (metadata, *md);
				}

				add_metadata_to_hash (table, metadata);
				g_object_unref (metadata);
			}
		}

		set_file_info_from_hash (info, table);
		g_hash_table_unref (table);
	}

	set_attributes_from_tagsets (info, update_general_attributes);
}


/*
 * exiv2_read_metadata_from_file
 * reads metadata from image files
 * code relies heavily on example1 from the exiv2 website
 * http://www.exiv2.org/example1.html
 */
extern "C"
gboolean
exiv2_read_metadata_from_file (GFile         *file,
			       GFileInfo     *info,
			       gboolean       update_general_attributes,
			       GCancellable  *cancellable,
			       GError       **error)
{
	try {
		char *path;

		path = g_file_get_path (file);
		if (path == NULL) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(path);
		g_free (path);

		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}
		// Set the log level to only show errors (and suppress warnings, informational and debug messages)
		Exiv2::LogMsg::setLevel(Exiv2::LogMsg::error);
		exiv2_read_metadata (image, info, update_general_attributes);
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_read_metadata_from_buffer (void       *buffer,
				 gsize       buffer_size,
				 GFileInfo  *info,
				 gboolean    update_general_attributes,
				 GError    **error)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) buffer, buffer_size);

		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		exiv2_read_metadata (image, info, update_general_attributes);
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


extern "C"
GFile *
exiv2_get_sidecar (GFile *file)
{
	char  *uri;
	char  *uri_wo_ext;
	char  *sidecar_uri;
	GFile *sidecar;

	uri = g_file_get_uri (file);
	uri_wo_ext = _g_uri_remove_extension (uri);
	sidecar_uri = g_strconcat (uri_wo_ext, ".xmp", NULL);
	sidecar = g_file_new_for_uri (sidecar_uri);

	g_free (sidecar_uri);
	g_free (uri_wo_ext);
	g_free (uri);

	return sidecar;
}


extern "C"
gboolean
exiv2_read_sidecar (GFile     *file,
		    GFileInfo *info,
		    gboolean   update_general_attributes)
{
	try {
		char *path;

		path = g_file_get_path (file);
		if (path == NULL)
			return FALSE;

		Exiv2::DataBuf buf = Exiv2::readFile(path);
		g_free (path);

		std::string xmpPacket;
		xmpPacket.assign(reinterpret_cast<char*>(buf.pData_), buf.size_);
		Exiv2::XmpData xmpData;

		if (0 != Exiv2::XmpParser::decode(xmpData, xmpPacket))
			return FALSE;

		if (! xmpData.empty()) {
			GHashTable *table = create_metadata_hash ();

			Exiv2::XmpData::iterator end = xmpData.end();
			for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
				stringstream raw_value;
				raw_value << md->value();

				stringstream description;
				if (! md->tagLabel().empty())
					description << md->tagLabel();
				else
					description << md->groupName() << "." << md->tagName();

				GthMetadata *metadata;
				metadata = create_metadata (md->key().c_str(),
							    description.str().c_str(),
							    md->print().c_str(),
							    raw_value.str().c_str(),
							    "Xmp::Sidecar",
							    md->typeName());
				if (metadata != NULL) {
					if ((g_strcmp0 (md->typeName(), "XmpBag") == 0)
					    || (g_strcmp0 (md->typeName(), "XmpSeq") == 0))
					{
						add_string_list_to_metadata (metadata, *md);
					}

					add_metadata_to_hash (table, metadata);
					g_object_unref (metadata);
				}
			}

			set_file_info_from_hash (info, table);
			g_hash_table_unref (table);
		}
		Exiv2::XmpParser::terminate();

		set_attributes_from_tagsets (info, update_general_attributes);
	}
	catch (Exiv2::AnyError& e) {
		std::cerr << "Caught Exiv2 exception '" << e << "'\n";
		return FALSE;
	}

	return TRUE;
}


static void
mandatory_int (Exiv2::ExifData &checkdata,
	       const char      *tag,
	       int              value)
{
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}


static void
mandatory_string (Exiv2::ExifData &checkdata,
		  const char      *tag,
		  const char      *value)
{
	Exiv2::ExifKey key = Exiv2::ExifKey(tag);
	if (checkdata.findKey(key) == checkdata.end())
		checkdata[tag] = value;
}


const char *
gth_main_get_metadata_type (gpointer    metadata,
			    const char *attribute)
{
	const char      *value_type = NULL;
	GthMetadataInfo *metadatum_info;

	if (GTH_IS_METADATA (metadata)) {
		value_type = gth_metadata_get_value_type (GTH_METADATA (metadata));
		if ((g_strcmp0 (value_type, "Undefined") == 0) || (g_strcmp0 (value_type, "") == 0))
			value_type = NULL;

		if (value_type != NULL)
			return value_type;
	}

	metadatum_info = gth_main_get_metadata_info (attribute);
	if (metadatum_info != NULL)
		value_type = metadatum_info->type;

	return value_type;
}


#if 0


static void
dump_exif_data (Exiv2::ExifData &exifData,
		const char      *prefix)
{
	std::cout << prefix << "\n";

	try {
		if (exifData.empty()) {
#if EXIV2_TEST_VERSION(0, 27, 0)
			throw Exiv2::Error(Exiv2::kerErrorMessage, " No Exif data found in the file");
#else
			throw Exiv2::Error(1, " No Exif data found in the file");
#endif
		}
		Exiv2::ExifData::const_iterator end = exifData.end();
		for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
			const char* tn = i->typeName();
			std::cout << std::setw(44) << std::setfill(' ') << std::left
				  << i->key() << " "
				  << "0x" << std::setw(4) << std::setfill('0') << std::right
				  << std::hex << i->tag() << " "
				  << std::setw(9) << std::setfill(' ') << std::left
				  << (tn ? tn : "Unknown") << " "
				  << std::dec << std::setw(3)
				  << std::setfill(' ') << std::right
				  << i->count() << "  "
				  << std::dec << i->value()
				  << "\n";
		}
		std::cout << "\n";
	}
	catch (Exiv2::Error& e) {
	    std::cout << "Caught Exiv2 exception '" << e.what() << "'\n";
	    return;
	}
}


#endif


static Exiv2::DataBuf
exiv2_write_metadata_private (Exiv2::Image::AutoPtr  image,
			      GFileInfo             *info,
			      GthImage              *image_data)
{
	static char  *software_name = NULL;
	char        **attributes;
	int           i;

	image->clearMetadata();

	// EXIF Data

	Exiv2::ExifData ed;
	attributes = g_file_info_list_attributes (info, "Exif");
	for (i = 0; attributes[i] != NULL; i++) {
		GthMetadata *metadatum;
		char *key;

		metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		key = exiv2_key_from_attribute (attributes[i]);

		try {
			/* If the metadatum has no value yet, a new empty value
			 * is created. The type is taken from Exiv2's tag
			 * lookup tables. If the tag is not found in the table,
			 * the type defaults to ASCII.
			 * We always create the metadatum explicitly if the
			 * type is available to avoid type errors.
			 * See bug #610389 for more details.  The original
			 * explanation is here:
			 * http://uk.groups.yahoo.com/group/exiv2/message/1472
			 */

			const char *raw_value = gth_metadata_get_raw (metadatum);
			const char *value_type = gth_main_get_metadata_type (metadatum, attributes[i]);

			if ((raw_value != NULL) && (strcmp (raw_value, "") != 0) && (value_type != NULL)) {
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				value->read (raw_value);
				Exiv2::ExifKey exif_key(key);
				ed.add (exif_key, value.get());
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	g_strfreev (attributes);

	// Mandatory tags - add if not already present

	mandatory_int (ed, "Exif.Image.XResolution", 72);
	mandatory_int (ed, "Exif.Image.YResolution", 72);
	mandatory_int (ed, "Exif.Image.ResolutionUnit", 2);
	mandatory_int (ed, "Exif.Image.YCbCrPositioning", 1);
	mandatory_int (ed, "Exif.Photo.ColorSpace", 1);
	mandatory_string (ed, "Exif.Photo.ExifVersion", "48 50 50 49");
	mandatory_string (ed, "Exif.Photo.ComponentsConfiguration", "1 2 3 0");
	mandatory_string (ed, "Exif.Photo.FlashpixVersion", "48 49 48 48");

	// Overwrite the software tag if the image content was modified

	if (g_file_info_get_attribute_boolean (info, "gth::file::image-changed")) {
		if (software_name == NULL)
			software_name = g_strconcat (g_get_application_name (), " ", PACKAGE_VERSION, NULL);
		ed["Exif.Image.ProcessingSoftware"] = software_name;
	}

	// Update the dimension tags with actual image values

	cairo_surface_t *surface = NULL;
	int width = 0;
	int height = 0;

	if (image_data != NULL)
		surface = gth_image_get_cairo_surface (image_data);

	if (surface != NULL) {
		width = cairo_image_surface_get_width (surface);
		if (width > 0) {
			ed["Exif.Photo.PixelXDimension"] = width;
			ed["Exif.Image.ImageWidth"] = width;
		}

		height = cairo_image_surface_get_height (surface);
		if (height > 0) {
			ed["Exif.Photo.PixelYDimension"] = height;
			ed["Exif.Image.ImageLength"] = height;
		}

		ed["Exif.Image.Orientation"] = 1;
	}

	// Update the thumbnail

	Exiv2::ExifThumb thumb(ed);
	if ((surface != NULL) && (width > 0) && (height > 0)) {
		cairo_surface_t *thumbnail;
		GthImage        *thumbnail_data;
		char            *buffer;
		gsize            buffer_size;

		scale_keeping_ratio (&width, &height, 128, 128, FALSE);
		thumbnail = _cairo_image_surface_scale (surface, width, height, SCALE_FILTER_BEST, NULL);
		thumbnail_data = gth_image_new_for_surface (thumbnail);
		if (gth_image_save_to_buffer (thumbnail_data,
					      "image/jpeg",
					      NULL,
					      &buffer,
					      &buffer_size,
					      NULL,
					      NULL))
		{
			thumb.setJpegThumbnail ((Exiv2::byte *) buffer, buffer_size);
			ed["Exif.Thumbnail.XResolution"] = 72;
			ed["Exif.Thumbnail.YResolution"] = 72;
			ed["Exif.Thumbnail.ResolutionUnit"] =  2;
			g_free (buffer);
		}
		else
			thumb.erase();

		g_object_unref (thumbnail_data);
		cairo_surface_destroy (thumbnail);
	}
	else
		thumb.erase();

	if (surface != NULL)
		cairo_surface_destroy (surface);

	// Update the DateTime tag

	if (g_file_info_get_attribute_object (info, "Exif::Image::DateTime") == NULL) {
		GTimeVal current_time;
		g_get_current_time (&current_time);
		char *date_time = _g_time_val_to_exif_date (&current_time);
		ed["Exif.Image.DateTime"] = date_time;
		g_free (date_time);
	}
	ed.sortByKey();

	// IPTC Data

	Exiv2::IptcData id;
	attributes = g_file_info_list_attributes (info, "Iptc");
	for (i = 0; attributes[i] != NULL; i++) {
		gpointer  metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char     *key = exiv2_key_from_attribute (attributes[i]);

		try {
			const char *value_type;

			value_type = gth_main_get_metadata_type (metadatum, attributes[i]);
			if (value_type != NULL) {
				/* See the exif data code above for an explanation. */
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				Exiv2::IptcKey iptc_key(key);

				const char *raw_value;

				switch (gth_metadata_get_data_type (GTH_METADATA (metadatum))) {
				case GTH_METADATA_TYPE_STRING:
					raw_value = gth_metadata_get_raw (GTH_METADATA (metadatum));
					if ((raw_value != NULL) && (strcmp (raw_value, "") != 0)) {
						value->read (raw_value);
						id.add (iptc_key, value.get());
					}
					break;

				case GTH_METADATA_TYPE_STRING_LIST:
					GthStringList *string_list = gth_metadata_get_string_list (GTH_METADATA (metadatum));
					for (GList *scan = gth_string_list_get_list (string_list); scan; scan = scan->next) {
						char *single_value = (char *) scan->data;

						value->read (single_value);
						id.add (iptc_key, value.get());
					}
					break;
				}
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	id.sortByKey();
	g_strfreev (attributes);

	// XMP Data

	Exiv2::XmpData xd;
	attributes = g_file_info_list_attributes (info, "Xmp");
	for (i = 0; attributes[i] != NULL; i++) {
		gpointer  metadatum = (GthMetadata *) g_file_info_get_attribute_object (info, attributes[i]);
		char     *key = exiv2_key_from_attribute (attributes[i]);

		try {
			const char *value_type;

			value_type = gth_main_get_metadata_type (metadatum, attributes[i]);
			if (value_type != NULL) {
				/* See the exif data code above for an explanation. */
				Exiv2::Value::AutoPtr value = Exiv2::Value::create (Exiv2::TypeInfo::typeId (value_type));
				Exiv2::XmpKey xmp_key(key);

				const char *raw_value;

				switch (gth_metadata_get_data_type (GTH_METADATA (metadatum))) {
				case GTH_METADATA_TYPE_STRING:
					raw_value = gth_metadata_get_raw (GTH_METADATA (metadatum));
					if ((raw_value != NULL) && (strcmp (raw_value, "") != 0)) {
						value->read (raw_value);
						xd.add (xmp_key, value.get());
					}
					break;

				case GTH_METADATA_TYPE_STRING_LIST:
					GthStringList *string_list = gth_metadata_get_string_list (GTH_METADATA (metadatum));
					for (GList *scan = gth_string_list_get_list (string_list); scan; scan = scan->next) {
						char *single_value = (char *) scan->data;

						value->read (single_value);
						xd.add (xmp_key, value.get());
					}
					break;
				}
			}
		}
		catch (Exiv2::AnyError& e) {
			/* we don't care about invalid key errors */
			g_warning ("%s", e.what());
		}

		g_free (key);
	}
	xd.sortByKey();
	g_strfreev (attributes);

	try {
		image->setExifData(ed);
		image->setIptcData(id);
		image->setXmpData(xd);
		image->writeMetadata();
	}
	catch (Exiv2::AnyError& e) {
		g_warning ("%s", e.what());
	}

	Exiv2::BasicIo &io = image->io();
	io.open();

	return io.read(io.size());
}


extern "C"
gboolean
exiv2_supports_writes (const char *mime_type)
{
	return (g_content_type_equals (mime_type, "image/jpeg")
		|| g_content_type_equals (mime_type, "image/tiff")
		|| g_content_type_equals (mime_type, "image/png"));
}


extern "C"
gboolean
exiv2_write_metadata (GthImageSaveData *data)
{
	if (exiv2_supports_writes (data->mime_type) && (data->file_data != NULL)) {
		try {
			Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) data->buffer, data->buffer_size);
			g_assert (image.get() != 0);

			Exiv2::DataBuf buf = exiv2_write_metadata_private (image, data->file_data->info, data->image);

			g_free (data->buffer);
			data->buffer = g_memdup (buf.pData_, buf.size_);
			data->buffer_size = buf.size_;
		}
		catch (Exiv2::AnyError& e) {
			if (data->error != NULL)
				*data->error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
			g_warning ("%s\n", e.what());
			return FALSE;
		}
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_write_metadata_to_buffer (void      **buffer,
				gsize      *buffer_size,
				GFileInfo  *info,
				GthImage   *image_data,
				GError    **error)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) *buffer, *buffer_size);
		g_assert (image.get() != 0);

		Exiv2::DataBuf buf = exiv2_write_metadata_private (image, info, image_data);

		g_free (*buffer);
		*buffer = g_memdup (buf.pData_, buf.size_);
		*buffer_size = buf.size_;
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


extern "C"
gboolean
exiv2_clear_metadata (void   **buffer,
		      gsize   *buffer_size,
		      GError **error)
{
	try {
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open ((Exiv2::byte*) *buffer, *buffer_size);

		if (image.get() == 0) {
			if (error != NULL)
				*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, _("Invalid file format"));
			return FALSE;
		}

		try {
			image->clearMetadata();
			image->writeMetadata();
		}
		catch (Exiv2::AnyError& e) {
			g_warning ("%s", e.what());
		}

		Exiv2::BasicIo &io = image->io();
		io.open();
		Exiv2::DataBuf buf = io.read(io.size());

		g_free (*buffer);
		*buffer = g_memdup (buf.pData_, buf.size_);
		*buffer_size = buf.size_;
	}
	catch (Exiv2::AnyError& e) {
		if (error != NULL)
			*error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_FAILED, e.what());
		return FALSE;
	}

	return TRUE;
}


#define MAX_RATIO_ERROR_TOLERANCE 0.01


GdkPixbuf *
exiv2_generate_thumbnail (const char *uri,
			  const char *mime_type,
			  int         requested_size)
{
	GdkPixbuf *pixbuf = NULL;

	if (! _g_content_type_is_a (mime_type, "image/jpeg")
	    && ! _g_content_type_is_a (mime_type, "image/tiff"))
	{
		return NULL;
	}

	try {
		char *path;

		path = g_filename_from_uri (uri, NULL, NULL);
		if (path == NULL)
			return NULL;

		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open (path);
		image->readMetadata ();
		Exiv2::ExifThumbC exifThumb (image->exifData ());
		Exiv2::DataBuf thumb = exifThumb.copy ();

		g_free (path);

		if (thumb.pData_ == NULL)
			return NULL;

		Exiv2::ExifData &ed = image->exifData();

		long orientation = (ed["Exif.Image.Orientation"].count() > 0) ? ed["Exif.Image.Orientation"].toLong() : 1;
		long image_width = (ed["Exif.Photo.PixelXDimension"].count() > 0) ? ed["Exif.Photo.PixelXDimension"].toLong() : -1;
		long image_height = (ed["Exif.Photo.PixelYDimension"].count() > 0) ? ed["Exif.Photo.PixelYDimension"].toLong() : -1;

		if ((orientation != 1) || (image_width <= 0) || (image_height <= 0))
			return NULL;

		GInputStream *stream = g_memory_input_stream_new_from_data (thumb.pData_, thumb.size_, NULL);
		pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
		g_object_unref (stream);

		if (pixbuf == NULL)
			return NULL;

		/* Heuristic to find out-of-date thumbnails: the thumbnail and image aspect ratios must be equal */

		int    pixbuf_width = gdk_pixbuf_get_width (pixbuf);
		int    pixbuf_height = gdk_pixbuf_get_height (pixbuf);
		double image_ratio = (((double) image_width) / image_height);
		double thumbnail_ratio = (((double) pixbuf_width) / pixbuf_height);
		double ratio_delta = (image_ratio > thumbnail_ratio) ? (image_ratio - thumbnail_ratio) : (thumbnail_ratio - image_ratio);

		if ((ratio_delta > MAX_RATIO_ERROR_TOLERANCE) /* the tolerance is used because the reduced image can have a slightly different ratio due to rounding errors */
		    || (MAX (pixbuf_width, pixbuf_height) < requested_size)) /* ignore the embedded image if it's too small compared to the requested size */
		{
			g_object_unref (pixbuf);
			return NULL;
		}

		/* Scale the pixbuf to perfectly fit the requested size */

		if (scale_keeping_ratio (&pixbuf_width,
					 &pixbuf_height,
					 requested_size,
					 requested_size,
					 TRUE))
		{
			GdkPixbuf *tmp = pixbuf;
			pixbuf = _gdk_pixbuf_scale_simple_safe (tmp, pixbuf_width, pixbuf_height, GDK_INTERP_BILINEAR);
			g_object_unref (tmp);
		}

		/* Save the original image size in the pixbuf options */

		char *s = g_strdup_printf ("%ld", image_width);
		gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Width", s);
		g_object_set_data (G_OBJECT (pixbuf), "gnome-original-width", GINT_TO_POINTER ((int) image_width));
		g_free (s);

		s = g_strdup_printf ("%ld", image_height);
		gdk_pixbuf_set_option (pixbuf, "tEXt::Thumb::Image::Height", s);
		g_object_set_data (G_OBJECT (pixbuf), "gnome-original-height", GINT_TO_POINTER ((int) image_height));
		g_free (s);

		/* Set the orientation option to correctly rotate the thumbnail
		 * in gnome_desktop_thumbnail_factory_generate_thumbnail() */

		char *orientation_s = g_strdup_printf ("%ld", orientation);
		gdk_pixbuf_set_option (pixbuf, "orientation", orientation_s);
		g_free (orientation_s);
	}
	catch (Exiv2::AnyError& e) {
	}

	return pixbuf;
}
