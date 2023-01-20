/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include "glib-utils.h"
#include "gth-main.h"
#include "gth-metadata-provider.h"
#include "gth-metadata-provider-file.h"


GthMetadataCategory file_metadata_category[] = {
	{ "file", N_("File"), 1 },
	{ "general", N_("Metadata"), 2 },
	{ NULL, NULL, 0 }
};


GthMetadataInfo file_metadata_info[] = {
	{ "standard::display-name", N_("Name"), "file", 1, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "gth::file::display-size", N_("Size"), "file", 2, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "standard::size", N_("Bytes"), "file", 3, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	/* Translators: the file modified time. */
	{ "gth::file::display-mtime", N_("Modified Date & Time"), "file", 4, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "standard::fast-content-type", N_("Type"), "file", 5, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "gth::file::is-modified", NULL, "file", 6, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "gth::file::full-name", N_("Location"), "file", 7, NULL, GTH_METADATA_ALLOW_IN_PRINT | GTH_METADATA_ALLOW_IN_FILE_LIST | GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "general::format", N_("Format"), "file", 10, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },
	{ "general::dimensions", N_("Dimensions"), "file", 12, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "general::duration", N_("Duration"), "file", 11, NULL, GTH_METADATA_ALLOW_EVERYWHERE },

	{ "Embedded::Photo::Exposure", N_("Exposure Settings"), "general", 10, NULL, GTH_METADATA_ALLOW_IN_PRINT | GTH_METADATA_ALLOW_IN_FILE_LIST  },
	{ "Embedded::Photo::Aperture", N_("Aperture"), "general", 11, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::ISOSpeed", N_("ISO Speed"), "general", 12, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::ExposureTime", N_("Exposure Time"), "general", 13, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::ShutterSpeed", N_("Shutter Speed"), "general", 14, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::FocalLength", N_("Focal Length"), "general", 16, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::Flash", N_("Flash"), "general", 17, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::CameraModel", N_("Camera Model"), "general", 18, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Loaded::Image::ColorProfile", N_("Color Profile"), "general", 19, NULL, GTH_METADATA_ALLOW_IN_PROPERTIES_VIEW },

	{ "general::datetime", N_("General Date & Time"), "general", 20, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "general::title", N_("Title"), "general", 21, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "general::location", N_("Place"), "general", 22, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "general::description", N_("Description"), "general", 23, NULL, GTH_METADATA_ALLOW_IN_PRINT },
	{ "general::tags", N_("Tags"), "general", 24, NULL, GTH_METADATA_ALLOW_EVERYWHERE },
	{ "general::rating", N_("Rating"), "general", 25, NULL, GTH_METADATA_ALLOW_EVERYWHERE },

	{ "Embedded::Photo::Author", N_("Author"), "general", 30, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },
	{ "Embedded::Photo::Copyright", N_("Copyright"), "general", 31, NULL, GTH_METADATA_ALLOW_EVERYWHERE  },

	{ "gth::file::emblems", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "gth::standard::secondary-sort-order", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "image::width", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "image::height", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "frame::width", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "frame::height", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Image::Orientation", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },
	{ "Embedded::Photo::DateTimeOriginal", "", "", 0, NULL, GTH_METADATA_ALLOW_NOWHERE },

	{ NULL, NULL, NULL, 0, 0 }
};


void
gth_main_register_default_metadata (void)
{
	gth_main_register_metadata_category (file_metadata_category);
	gth_main_register_metadata_info_v (file_metadata_info);
	gth_main_register_metadata_provider (GTH_TYPE_METADATA_PROVIDER_FILE);
}
